#include "Player.h"
#include "HUD.h"
#include "Qor/Sound.h"
#include "Qor/Qor.h"
#include "Qor/Physics.h"
#include "Qor/Particle.h"
#include "Qor/Material.h"
#include "GameState.h"
#include "Qor/BasicPartitioner.h"
using namespace std;
using namespace glm;

const unsigned Player :: MAX_DECALS = 32;

Player :: Player(
    GameState* state,
    Node* root,
    shared_ptr<Controller> controller,
    Cache<Resource, string>* cache,
    Physics* physics,
    Window* window,
    Qor* engine,
    GameSpec* spec,
    std::function<bool()> lock_if
):
    m_pState(state),
    m_pRoot(root),
    m_pOrthoRoot(make_shared<Node>()),
    m_pController(controller),
    m_pCache(cache),
    m_pPhysics(physics),
    m_pWindow(window),
    m_pQor(engine),
    m_pGameSpec(spec),
    m_WeaponStash(spec->weapons()),
    m_FlashAlarm(state->timeline())
    //m_LockIf(lock_if)
{
    auto _this = this;
    
    m_pOrthoCamera = make_shared<Camera>(m_pQor->resources(), m_pQor->window());
    m_pOrthoCamera->ortho();
    m_pOrthoRoot->add(m_pOrthoCamera);
    m_pHUD = make_shared<HUD>(this, window, controller->input(), cache);
    m_pOrthoRoot->add(m_pHUD);
    m_pPlayerMesh = make_shared<Mesh>();
    m_pPlayerMesh->set_box(Box(
        vec3(-0.2f, -0.6f, -0.2f),
        vec3(0.2f, 0.6f, 0.2f)
    ));
    m_pPlayerMesh->set_physics(Node::Physics::DYNAMIC);
    m_pPlayerMesh->set_physics_shape(Node::CAPSULE);
    m_pPlayerMesh->friction(0.0f);
    m_pPlayerMesh->mass(80.0f);
    m_pPlayerMesh->inertia(false);
    m_pCamera = make_shared<Camera>(cache, window);
    m_fFOV = m_pCamera->fov();
    //m_pRoot->add(m_pCamera);
    m_pPlayerMesh->add(m_pCamera);
    m_pCamera->position(vec3(0.0f, 0.6f, 0.0f));
    m_pRoot->add(m_pPlayerMesh);
    m_pPlayerMesh->config()->set<int>("hp", 10);
    m_pPlayerMesh->config()->set<int>("maxhp", 10);
    
    auto hp_change = [_this](){
        int value = _this->m_pPlayerMesh->config()->at<int>("hp");
        int maxvalue = _this->m_pPlayerMesh->config()->at<int>("maxhp");
        _this->m_pHUD->hp(kit::round_int(100.0f * value / maxvalue));
    };
    m_pPlayerMesh->config()->on_change("hp",hp_change);
    hp_change();

    m_pDecal = cache->cache_cast<ITexture>("decal_bullethole1.png");
    m_pSpark = cache->cache_cast<ITexture>("spark.png");
    m_pController = m_pQor->session()->profile(0)->controller();

    m_WeaponStash.give_all();
    m_WeaponStash.next();
    refresh_weapon();
    update_hud();
    
    m_pInterface = kit::init_shared<PlayerInterface3D>(
        m_pController,
        m_pCamera,
        m_pPlayerMesh,
        m_pQor->session()->profile(0)->config(),
        [_this]{
            return (_this->m_LockIf && _this->m_LockIf()) || _this->dead();
        }
    );
    m_pInterface->speed(12.0f);
    
    //btRigidBody* pmesh_body = (btRigidBody*)m_pPlayerMesh->body()->body();
    auto pmesh = m_pPlayerMesh.get();
    auto camera = m_pCamera.get();
    auto interface = m_pInterface.get();
    m_pPhysics->on_generate([_this, physics, pmesh,interface,cache,camera]{
        auto pmesh_body = (btRigidBody*)pmesh->body()->body();
        pmesh_body->setActivationState(DISABLE_DEACTIVATION);
        //pmesh_body->setAngularFactor(btVector3(0,0,0));
        pmesh_body->setCcdMotionThreshold(1.0f);
        //pmesh_body->setGravity();
        //pmesh_body->setCcdSweptSphereRadius(0.25f);
        //pmesh_body->setRestitution(0.0f);
        //pmesh_body->setDamping(0.0f, 0.0f);
        ////pmesh_body->setRestitution(0.0f);
        interface->on_jump([_this, physics, pmesh_body, cache, camera]{
            if(not _this->can_jump())
                return;
            pmesh_body->applyCentralImpulse(
                btVector3(0.0f, 1000.0f, 0.0f)
            );
            Sound::play(camera, "jump.wav", cache);
        });
    });
    m_pPlayerMesh->move(vec3(0.0f, 0.6f, 0.0f));

    m_pCamera->perspective();
    m_pCamera->listen();
    
    //m_pInterface->fly();
    //btRigidBody* pmesh_body = (btRigidBody*)m_pPlayerMesh->body()->body();

    m_bScope = true;
    scope(false);

    auto hud = m_pHUD.get();
    m_pState->event("message",[hud](const std::shared_ptr<Meta>& m){
        hud->message(m->at<string>("message"), Color(m->at<string>("color", string("FFFFFF"))));
    });

    state->partitioner()->register_object(m_pPlayerMesh, 0);
}

Player :: ~Player()
{
    m_pPlayerMesh->detach();
    //m_pInterface->unplug();
}

void Player :: update_hud()
{
    if(m_WeaponStash.active()) {
        int value = -1;
        if(m_WeaponStash.active()->spec()->clip() > 0) // 
            value = m_WeaponStash.active()->clip();
        int maxvalue = m_WeaponStash.active()->ammo();
        m_pHUD->ammo(value, maxvalue);
    }
    else
        m_pHUD->ammo(-1,-1);
}

void Player :: scope(bool b)
{
    if(m_bScope == b)
        return;
    
    if(b)
    {
        auto tex = m_pQor->resources()->cache_cast<Texture>("scope_overlay.png");
        if(m_pCrosshair) m_pCrosshair->detach();
        m_pCrosshair = make_shared<Mesh>(
            make_shared<MeshGeometry>(
                Prefab::quad(
                    vec2(0.0f, 0.0f),
                    vec2((float)m_pWindow->size().x, (float)m_pWindow->size().y)
                )
            )
        );
        m_pCrosshair->add_modifier(make_shared<Wrap>(Prefab::quad_wrap(
            vec2(1.0f, -1.0f)
        )));
        m_pCrosshair->material(make_shared<MeshMaterial>(tex));
        m_pOrthoRoot->add(m_pCrosshair);

        m_pViewModel->each([](Node* n){
            n->self_visible(false);
        });
    }
    else
    {
        auto tex = m_pQor->resources()->cache_as<Material>("crosshair2.png");
        tex->diffuse(Color(1.0f, 1.0f, 1.0f, 0.5f));
        if(m_pCrosshair) m_pCrosshair->detach();
        m_pCrosshair = make_shared<Mesh>(
            make_shared<MeshGeometry>(
                Prefab::quad(
                    -vec2((float)tex->center().x, (float)tex->center().y) / 2.0f,
                    vec2((float)tex->center().x, (float)tex->center().y) / 2.0f
                )
            )
        );
        m_pCrosshair->add_modifier(make_shared<Wrap>(Prefab::quad_wrap(
            vec2(1.0f, -1.0f)
        )));
        m_pCrosshair->material(make_shared<MeshMaterial>(tex));
        m_pCrosshair->position(glm::vec3(m_pWindow->center().x, m_pWindow->center().y, 0.0f));
        m_pOrthoRoot->add(m_pCrosshair);
        
        m_pViewModel->each([](Node* n){
            n->self_visible(true);
        });
    }
    m_bScope = b;
}

void Player :: logic(Freq::Time t)
{
    if(not m_bEnter){
        Sound::play(m_pCamera.get(), "spawn.wav", m_pQor->resources());
        m_bEnter = true;
    }
    
    int hp = m_pPlayerMesh->config()->at<int>("hp");
    
    if(not hp)
        return;
    
    auto _this = this;
    auto pmesh = m_pPlayerMesh.get();
    auto pmesh_body = (btRigidBody*)pmesh->body()->body();
    pmesh_body->applyCentralForce(
        Physics::toBulletVector(glm::vec3(0.0f, -31.0f * pmesh->mass(), 0.0f))
    );
    
    if(m_LockIf && m_LockIf())
        return;
    
    if(m_pController->input()->key(SDLK_z).pressed_now()) {
        hurt(1);
    }
    
    if(m_pController->input()->key(SDLK_x).pressed_now()) {
        m_pState->event("message", make_shared<Meta>(MetaFormat::JSON,
            R"({"message": "RED TEAM SCORES", "color": "FF0000"})"
        ));
    }
    if(m_pController->input()->key(SDLK_c).pressed_now()) {
        m_pState->event("message", make_shared<Meta>(MetaFormat::JSON,
            R"({"message": "BLUE TEAM SCORES", "color": "0000FF"})"
        ));
    }
    
    if(m_pController->button("zoom").pressed_now()){
        bool zoomed = not m_pViewModel->zoomed(); // opposite of what it is now
        auto wpn = m_WeaponStash.active()->spec();
        if(!zoomed || not wpn->scope()){
            scope(false);
        }
        auto vm = m_pViewModel.get();
        if(wpn->scope())
            Sound::play(m_pCamera.get(), "zoom.wav", m_pQor->resources());
        m_pViewModel->zoom(zoomed, [zoomed,vm,_this]{
            if(zoomed) {
                auto wpn = _this->m_WeaponStash.active()->spec();
                _this->scope(wpn->scope());
                //if(wpn->scope()){
                //    vm->each([](Node* n){
                //        n->self_visible(false);
                //    });
                //}
            }
        });
    }

    if(m_pController->button("use").pressed_now())
    {
        auto hit = m_pPhysics->first_hit(
            m_pCamera->position(Space::WORLD),
            m_pCamera->to_world(vec3(0.0f, 0.0f, -3.0f))
        );
        Node* n = std::get<0>(hit);
        if(n)
        {
            n = n->subroot();
            if(n->has_event("use")){
                Sound::play(m_pCamera.get(), "switch.wav", m_pQor->resources());
                n->event("use", make_shared<Meta>());
            }
        }
    }


    for(int i=0; i<9; ++i)
    {
        if(not m_pViewModel->equipping()){
            if(m_pController->button(string("slot")+to_string(i))){
                if(m_WeaponStash.slot(i)){
                    Sound::play(m_pCamera.get(), "swap.wav", m_pQor->resources());
                    scope(false);
                    m_pViewModel->fast_zoom(false);
                    auto vm = m_pViewModel.get();
                    m_pViewModel->equip(false,[_this]{
                        _this->refresh_weapon();
                    });
                    break;
                }
            }
        }
    }
    
    if(m_pController->button("next").pressed_now()){
        if(m_pViewModel->idle()){
            if(m_WeaponStash.next()){
                Sound::play(m_pCamera.get(), "swap.wav", m_pQor->resources());
                scope(false);
                m_pViewModel->fast_zoom(false);
                auto vm = m_pViewModel.get();
                m_pViewModel->equip(false,[_this]{
                    _this->refresh_weapon();
                });
            }
        }
    }
    
    if(m_pController->button("previous").pressed_now()){
        if(m_pViewModel->idle()){
            if(m_WeaponStash.next(-1)){
                Sound::play(m_pCamera.get(), "swap.wav", m_pQor->resources());
                scope(false);
                m_pViewModel->fast_zoom(false);
                auto vm = m_pViewModel.get();
                m_pViewModel->equip(false,[_this]{
                    _this->refresh_weapon();
                });
            }
        }
    }
    
    if(m_pController->button("reload").pressed_now())
    {
        if(m_pViewModel->idle() && m_pViewModel->equipped()){
            //auto cache = m_pQor->resources();
            //auto camera = m_pCamera.get();
            if(m_WeaponStash.active()->can_reload()){
                scope(false);
                m_pViewModel->fast_zoom(false);
                auto vm = m_pViewModel.get();
                m_pViewModel->equip_time(Freq::Time(250));
                Sound::play(m_pCamera.get(), "reload.wav", m_pQor->resources());
                auto* weapon_stash = &m_WeaponStash;
                m_pViewModel->equip(false, [_this, vm, weapon_stash]{
                    weapon_stash->active()->reload();
                    vm->equip(true);
                    _this->update_hud();
                });
            }
        }
    }

    if(m_pController->button("fire") &&
       m_pViewModel->idle() &&
       m_pViewModel->equipped()
    ){
        //Sound::play(m_pCamera.get(), "shotgun.wav", m_pQor->resources());
        if(m_WeaponStash.active()->spec()->scope()){
            scope(false);
            m_pViewModel->zoom(false);
        }
        
        //Sound::play(m_pCamera.get(), "pump.wav", m_pQor->resources());
        //auto snd = m_pQor->make<Sound>("pump.wav");
        //auto time = make_shared<Freq::Time>();
        //auto sndptr = snd.get();
        //snd->on_tick.connect([sndptr,time](Freq::Time t){
        //    if(*time >= Freq::Time(250)) {
        //        if(not sndptr->played()){
        //            sndptr->detach_on_done();
        //            sndptr->play();
        //        }
        //    }else{
        //        *time += t;
        //    }
        //});
        //m_pCamera->add(snd);
        bool player_hit = false;
        m_pViewModel->recoil(Freq::Time(50), m_WeaponStash.active()->spec()->delay(), 0.05f);
        if(m_WeaponStash.active()->spec()->projectile().empty())
        {
            //auto p = m_pQor->make<Particle>("muzzleflash1.png");
            //m_pViewModel->add(p);
            //p->move(vec3(0.0f, 0.0f, -1.0f));
            //p->collapse(Space::WORLD);
             
            int burst = m_WeaponStash.active()->fire();
            update_hud();
            if(burst)
            {
                Sound::play(m_pCamera.get(), m_WeaponStash.active()->spec()->sound(), m_pQor->resources());
                
                for(int i=0; i<burst; ++i)
                {
                    auto mag_var = (rand() % 1000) * 0.001f * m_WeaponStash.active()->spec()->spread();
                    auto ang_var = (rand() % 1000) * 0.001f;
                    vec3 dir = glm::vec3(
                        cos(K_TAU * ang_var) * mag_var,
                        sin(K_TAU * ang_var) * mag_var,
                        -1.0f
                    );
                    dir = glm::normalize(dir);
                    
                    auto hit = m_pPhysics->first_hit(
                        m_pCamera->position(Space::WORLD),
                        m_pCamera->position(Space::WORLD) +
                            m_pCamera->orient_to_world(dir) * 100.0f
                    );
                    Node* n = std::get<0>(hit);
                    if(n)
                    {
                        //if(not n->hook("#player",Node::Hook::REVERSE).empty())
                        if(n->parent()->has_event("hit")){
                            auto hitinfo = make_shared<Meta>();
                            hitinfo->set<int>("damage", 1);
                            player_hit = true;
                            n->parent()->event("hit", hitinfo);
                        }
                        
                        decal(
                            std::get<1>(hit),
                            std::get<2>(hit),
                            Matrix::up(*m_pCamera->matrix(Space::WORLD)),
                            i * 0.0001
                        );
                    }
                }
            } else {
                Sound::play(m_pCamera.get(), "empty.wav", m_pQor->resources());
            }
            if(player_hit)
                Sound::play(m_pCamera.get(), "hit.wav", m_pQor->resources());
        }
        else
        {
            int burst = m_WeaponStash.active()->fire();
            update_hud();
            if(burst)
            {
                Sound::play(m_pCamera.get(), m_WeaponStash.active()->spec()->sound(), m_pQor->resources());
                
                // projectile
                auto m = m_pQor->make<Mesh>(m_WeaponStash.active()->spec()->projectile());
                m->set_physics(Node::DYNAMIC);
                m->set_physics_shape(Node::HULL);
                m->mass(1.0f);
                m_pViewModel->node()->add(m);
                m->move(glm::vec3(0.0f, 0.0f, -1.0f));
                m->collapse(Space::WORLD);
                auto dir = Matrix::heading(*m->matrix(Space::WORLD));
                m_pPhysics->generate(m.get());
                auto body = ((btRigidBody*)m->body()->body());
                body->applyCentralImpulse(Physics::toBulletVector(dir *
                    m_WeaponStash.active()->spec()->speed()
                ));
                auto vel = m->velocity();
                
                auto mp = m.get();
                auto cache = m_pCache;
                
                if(not m_WeaponStash.active()->spec()->gravity()) {
                    body->setGravity(btVector3(0.0f, 0.0f, 0.0f));
                    mp->inertia(false);
                    //body->setAngularFactor(btVector3(0.0f, 1.0f, 0.0f));
                    m->on_tick.connect([mp,vel,cache](Freq::Time){
                        auto body = ((btRigidBody*)mp->body()->body());
                        if(glm::dot(mp->velocity(),vel) < 1.0f - K_EPSILON ||
                            glm::length(mp->velocity()) != glm::length(vel))
                        {
                            auto snd = make_shared<Sound>(
                                cache->transform("explosion.wav"), cache
                            );
                            mp->stick(snd);
                            snd->detach_on_done();
                            snd->play();
                            mp->detach();
                        }
                    });
                }
            }else{
                Sound::play(m_pCamera.get(), "empty.wav", m_pQor->resources());
            }
        }
        
    }

    //LOGf("pos: %s, %s", t.s() % m_pPlayerMesh->position().y);

    //LOGf("children: %s", m_pRoot->num_subnodes());

    m_pViewModel->sway(m_pInterface->move() != glm::vec3(0.0f));
    m_pViewModel->sprint(
        m_pInterface->move() != glm::vec3(0.0f) && m_pInterface->sprint()
    );
    
    //m_pViewModel->position(m_pCamera->position(Space::WORLD));

    //auto input = m_pController->input();
    //if(input->key(SDLK_DOWN))
    //    m_pViewModel->model_move(glm::vec3(0.0f, -t.s() * 0.1f, 0.0f));
    //else if(input->key(SDLK_UP))
    //    m_pViewModel->model_move(glm::vec3(0.0f, t.s() * 0.1f, 0.0f));
    //else if(input->key(SDLK_LEFT))
    //    m_pViewModel->model_move(glm::vec3(-t.s() * 0.1f, 0.0f, 0.0f));
    //else if(input->key(SDLK_RIGHT))
    //    m_pViewModel->model_move(glm::vec3(t.s() * 0.1f, 0.0f, 0.0f));
    //else if(input->key(SDLK_w))
    //    m_pViewModel->model_move(glm::vec3(0.0f, 0.0f, t.s() * 0.1f));
    //else if(input->key(SDLK_r))
    //    m_pViewModel->model_move(glm::vec3(0.0f, 0.0f, -t.s() * 0.1f));

    //LOGf("model pos %s", Vector::to_string(m_pViewModel->model_pos()));
    //LOGf("zoomed model pos %s", Vector::to_string(m_pViewModel->zoomed_model_pos()));
    m_pHUD->fade(Color(
        m_FlashColor,
        (ceil(m_FlashAlarm.fraction_left() * 10)/10) * 0.5f
    ));

    if(m_pController->input()->key(SDLK_F12))
    {
        // pull up an f12 lawn chair
        m_pState->spectate();
        return;
    }
}

void Player :: decal(glm::vec3 contact, glm::vec3 normal, glm::vec3 up, float offset)
{
    const float decal_scale = 0.1f;
    auto m = make_shared<Mesh>(make_shared<MeshGeometry>(Prefab::quad(
        glm::vec2(decal_scale, decal_scale),
        glm::vec2(-decal_scale, -decal_scale)
    )));
    m->add_modifier(make_shared<Wrap>(Prefab::quad_wrap()));
    m->material(make_shared<MeshMaterial>(m_pDecal));
    auto right = glm::cross(normal, up);
    up = glm::cross(normal, right);
    *m->matrix() = glm::mat4(glm::orthonormalize(glm::mat3(
        right, up, normal
    )));
    m->position(contact);
    m->move(normal * (0.001f + offset));
    m->pend();
    m_Decals.push_back(m);
    while(m_Decals.size() > MAX_DECALS) {
        auto f = m_Decals.front();
        f->detach();
        m_Decals.pop_front();
    }
    m_pRoot->add(m);
    
    // spark
    auto m2 = make_shared<Mesh>(make_shared<MeshGeometry>(Prefab::quad(
        glm::vec2(decal_scale/2.0f, decal_scale/2.0f),
        glm::vec2(-decal_scale/2.0f, -decal_scale/2.0f)
    )));
    m2->add_modifier(make_shared<Wrap>(Prefab::quad_wrap()));
    m2->material(make_shared<MeshMaterial>(m_pSpark));
    *m2->matrix() = *m->matrix();
    m2->move(normal * (0.01f + offset));
    m2->pend();
    m_pRoot->add(m2);
    
    auto timer = make_shared<Freq::Timeline>();
    auto m2p = m2.get();
    m2->on_tick.connect([m2p, timer](Freq::Time t){
        timer->logic(t);
        if(timer->elapsed(Freq::Time::ms(50)))
            m2p->detach();
    });
}

bool Player :: can_jump() const
{
    auto pos = m_pPlayerMesh->position(Space::WORLD);
    auto jump_hit = m_pPhysics->first_hit(
        pos,
        pos - glm::vec3(0.0f, 0.6f + 0.2f, 0.0f)
    );
    Node* jump_hit_node = std::get<0>(jump_hit);
    return jump_hit_node;
}

void Player :: refresh_weapon()
{
    if(m_pViewModel)
        m_pViewModel->detach();
    
    if(m_WeaponStash.active())
    {
        auto wpn = m_WeaponStash.active()->spec();
        auto gun = m_pQor->make<Mesh>(wpn->model());
        m_pViewModel = make_shared<ViewModel>(m_pCamera, gun);
        gun->disable_physics();
        m_pRoot->add(m_pViewModel);
        m_pViewModel->fov(m_fFOV);
        m_pViewModel->model_pos(wpn->viewmodel_pos());
        m_pViewModel->zoomed_model_pos(wpn->viewmodel_zoomed_pos());
        m_pViewModel->reset_zoom();
        m_pViewModel->fast_equip(false);
        m_pViewModel->equip(true);
    }

    update_hud();
}

//void Player :: stand(vec3 pos)
//{
//    m_pPlayerMesh->position(pos);
//}

void Player :: die()
{
    m_pViewModel->equip(false);
    m_pPlayerMesh->config()->set<int>("hp", 0);
    Sound::play(m_pCamera.get(), "death.wav", m_pQor->resources());
}

void Player :: hurt(int dmg)
{
    int hp = m_pPlayerMesh->config()->at<int>("hp");
    hp = std::max(0,hp-dmg);
    m_pPlayerMesh->config()->set<int>("hp", hp);
    m_FlashColor = Color::red();
    m_FlashAlarm.set(Freq::Time::seconds(2.0f * dmg*1.0f/10));
    if(not hp)
        die();
    else
        Sound::play(m_pCamera.get(), "hurt.wav", m_pQor->resources());
}

bool Player :: alive()
{
    int hp = m_pPlayerMesh->config()->at<int>("hp");
    assert(hp >= 0);
    return hp > 0;
}
    
bool Player :: dead()
{
    int hp = m_pPlayerMesh->config()->at<int>("hp");
    assert(hp >= 0);
    return hp == 0;
}


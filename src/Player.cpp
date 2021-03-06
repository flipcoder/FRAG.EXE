#include "Player.h"
#include "HUD.h"
#include "Qor/Sound.h"
#include "Qor/Qor.h"
#include "Qor/Physics.h"
#include "Qor/Particle.h"
#include "Qor/Material.h"
#include "Game.h"
#include "Qor/BasicPartitioner.h"
#include "Qor/Profile.h"
#include "NetSpec.h"
#include <glm/gtx/matrix_interpolation.hpp>
#include <glm/gtx/orthonormalize.hpp>
using namespace std;
using namespace glm;

const unsigned Player :: MAX_DECALS = 32;

Player :: Player(
    Game* state,
    Node* root,
    shared_ptr<Profile> profile,
    //shared_ptr<Controller> controller,
    ResourceCache* cache,
    Physics* physics,
    Window* window,
    Qor* engine,
    GameSpec* spec,
    NetSpec* net,
    glm::vec3 pos, // position if no spawn point
    std::function<bool()> lock_if
):
    m_pState(state),
    m_pRoot(root),
    m_pOrthoRoot(make_shared<Node>()),
    m_pProfile(profile),
    m_pController(profile ? profile->controller() : nullptr),
    m_pCache(cache),
    m_pPhysics(physics),
    m_pWindow(window),
    m_pQor(engine),
    m_pSpec(spec),
    m_WeaponStash(spec->weapons()),
    m_FlashAlarm(state->timeline()),
    m_pNet(net),
    m_LockIf(lock_if),
    m_NetTransform(glm::mat4(1.0f)),
    m_HurtSoundAlarm(state->timeline()),
    m_KnockbackAlarm(state->timeline())
{
    auto _this = this;
    
    m_pOrthoCamera = make_shared<Camera>(m_pQor->resources(), m_pQor->window());
    m_pOrthoCamera->ortho();
    m_pOrthoRoot->add(m_pOrthoCamera);
    m_pHUD = make_shared<HUD>(this, window, m_pController ? m_pController->input() : nullptr, engine->session(), cache);
    m_pOrthoRoot->add(m_pHUD);
    m_pPlayerShape = make_shared<Mesh>();
    m_pPlayerShape->position(pos);
    // forward mesh gifts to this object
    //m_pPlayerShape->event("give", [_this](std::shared_ptr<Meta> m){
    //    _this->give(m);
    //});
    m_pPlayerShape->event("hit", [_this](std::shared_ptr<Meta> m){
        if(_this->dead())
            return;
            
        // splash damage
        if(m->has("dist"))
        {
            //LOG("splash");
            float dist = std::max<float>((float)m->at<double>("dist"),1.0f);
            float rad = (float)m->at<double>("radius");
            auto value = (float)m->at<int>("damage")*rad/(dist*dist*dist);
            //LOGf("value: %s", value);
            _this->hurt(kit::round_int(value));
            vec3 vec = m->at<vec3>("vec");
            _this->shape()->impulse(vec3(
                100.0f*vec.x,
                std::min<float>(1500.0f, 1000.0f*vec.y),
                100.0f*vec.z
            ));
            _this->knockback();
        }
        else{
            _this->hurt(m->at<int>("damage"));
        }
        if(_this->dead()){
            Player* owner = m->at<Player*>("owner", nullptr);
            if(owner)
                owner->add_frag(_this);
        }
    });
    m_StandBox = Box(
        vec3(-0.6f, -0.3f, -0.6f),
        vec3(0.6f, 0.3f, 0.6f)
    );
    m_CrouchBox = Box(
        vec3(-0.6f, -0.025f, -0.6f),
        vec3(0.6f, 0.025f, 0.6f)
    );
    m_pPlayerShape->set_box(m_StandBox);
    //if(m_pNet->server() || local())
        m_pPlayerShape->set_physics(Node::Physics::DYNAMIC);
    //else
        //m_pPlayerShape->set_physics(Node::Physics::KINEMATIC);
    m_pPlayerShape->set_physics_shape(Node::CAPSULE);
    m_pPlayerShape->friction(0.0f);
    m_pPlayerShape->mass(80.0f);
    m_pPlayerShape->inertia(false);
    m_pPlayerShape->add_tag("player");
    m_pPlayerShape->config()->set("player", (void*)this);
    m_pProfile->temp()->set<string>("team", rand()%2?"red":"blue");
    m_pProfile->temp()->set<int>("frags", 0);
    m_pProfile->temp()->set<int>("deaths", 0);
    //if(local())
    //    m_pProfile->temp()->set<string>("name", m_pProfile->name());
    //else
    //    m_pProfile->temp()->set<string>("name", "Bot");
    m_pCamera = make_shared<Camera>(cache, window);
    
    if(not local())
    {
        m_pPlayerModel = make_shared<Mesh>(m_pCache->transform("player.obj"), m_pCache);
        m_pPlayerModel->position(vec3(0.0f, -m_pPlayerShape->box().size().y, 0.0f));
        m_pPlayerModel->disable_physics();
        m_pPlayerShape->add(m_pPlayerModel);
    }
    
    m_fFOV = m_pCamera->fov();
    //m_pRoot->add(m_pCamera);
    m_pPlayerShape->add(m_pCamera);
    m_pCamera->position(vec3(0.0f, m_pPlayerShape->box().size().y / 2.5f, 0.0f));
    m_pRoot->add(m_pPlayerShape);
     
    m_pDecal = cache->cache_cast<ITexture>("decal_bullethole1.png");
    m_pSpark = cache->cache_cast<ITexture>("spark.png");

    m_WeaponStash.give("glock");
    m_WeaponStash.give("shotgun");
    //m_WeaponStash.give_all();
    m_WeaponStash.slot(3);
    refresh_weapon();
    update_hud();
    
    auto hp_change = [_this](){
        int value = _this->m_pProfile->temp()->at<int>("hp");
        int maxvalue = std::max(1,_this->m_pProfile->temp()->at<int>("maxhp"));
        _this->m_pHUD->hp(kit::round_int(100.0f * value / maxvalue));
    };
    m_pProfile->temp()->ensure("hp",0); // these will be reset anyway
    m_pProfile->temp()->ensure("maxhp",0);
    m_pProfile->temp()->ensure("frags",0);
    m_HPChangeCon = m_pProfile->temp()->on_change("hp",hp_change);
    
    if(m_pController) {
        m_pInterface = kit::init_shared<PlayerInterface3D>(
            m_pController,
            m_pCamera,
            m_pPlayerShape,
            m_pProfile->config(),
            [_this]{
                return (_this->m_LockIf && _this->m_LockIf())
                    || _this->dead()
                    || not _this->m_KnockbackAlarm.elapsed();
            }
        );
        m_pInterface->speed(16.0f);
        m_pInterface->allow_sprint(false);
    }
    
    //btRigidBody* pmesh_body = (btRigidBody*)m_pPlayerShape->body()->body();
    auto pmesh = m_pPlayerShape.get();
    auto camera = m_pCamera.get();
    auto interface_ = m_pInterface.get();
    m_pPhysics->on_generate([_this, physics, pmesh,interface_,cache,camera]{
        auto pmesh_body = (btRigidBody*)pmesh->body()->body();
        pmesh_body->setActivationState(DISABLE_DEACTIVATION);
        //pmesh_body->setAngularFactor(btVector3(0,0,0));
        pmesh_body->setCcdMotionThreshold(1.0f);
        //pmesh_body->setGravity();
        //pmesh_body->setCcdSweptSphereRadius(0.25f);
        //pmesh_body->setRestitution(0.0f);
        //pmesh_body->setDamping(0.0f, 0.0f);
        ////pmesh_body->setRestitution(0.0f);
        if(interface_){
            interface_->on_jump(std::bind(&Player::jump, _this));
            interface_->on_crouch([_this]{
                _this->crouch(true);
            });
            interface_->on_uncrouch([_this]{
                return _this->crouch(false);
            });

        }
    });
    m_pPlayerShape->move(vec3(0.0f, 0.8f, 0.0f));

    m_pCamera->perspective();
    if(not m_pProfile->dummy())
        m_pCamera->listen();
    
    //m_pInterface->fly();
    //btRigidBody* pmesh_body = (btRigidBody*)m_pPlayerShape->body()->body();

    m_bScope = true;
    scope(false);

    auto hud = m_pHUD.get();
    m_pState->event("message",[hud](const std::shared_ptr<Meta>& m){
        hud->message(m->at<string>("message"), Color(m->at<string>("color", string("FFFFFF"))));
    });

    state->partitioner()->register_object(m_pPlayerShape, 0);

    m_HurtSoundAlarm.set(Freq::Time::seconds(0.0f)); //elapse immediately
    m_KnockbackAlarm.set(Freq::Time::seconds(0.0f)); //elapse immediately
}

Player :: ~Player()
{
    clear();
}

void Player :: clear()
{
    m_pPlayerShape->detach();
    m_pViewModel->detach();
}

void Player :: jump()
{
    if(m_bCrouched){
        m_pInterface->crouch(false);
        return;
    }
    if(not can_jump())
        return;
    
    auto pmesh_body = (btRigidBody*)m_pPlayerShape->body()->body();
    pmesh_body->applyCentralImpulse(
        btVector3(0.0f, 1000.0f, 0.0f)
    );
    Sound::play(m_pCamera.get(), "jump.wav", m_pCache);
}

bool Player :: crouch(bool b)
{
    if(b == m_bCrouched)
        return false;
    m_pPlayerShape->clear_body();
    m_pPlayerShape->set_box(b ? m_CrouchBox : m_StandBox);
    m_pCamera->position(vec3(0.0f, m_pPlayerShape->box().size().y / 2.0f, 0.0f));
    m_pPhysics->generate(m_pPlayerShape.get());
    m_bCrouched = b;
    return true;
}

void Player :: update_hud()
{
    if(m_WeaponStash.active()) {
        int value = -1;
        if(m_WeaponStash.active()->spec()->clip() > 0)
            value = m_WeaponStash.active()->clip();
        int maxvalue = m_WeaponStash.active()->ammo();
        m_pHUD->ammo(value, maxvalue);
    }
    else
        m_pHUD->ammo(-1,-1);
    m_pHUD->dirty();
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
    if(m_pController && m_pController->input()->key(SDLK_F12))
    {
        // pull up an f12 lawn chair
        m_pSpec->spectate();
        return;
    }
    
    m_pOrthoRoot->logic(t);
   
    auto _this = this;
    auto pmesh = m_pPlayerShape.get();
    auto pmesh_body = (btRigidBody*)pmesh->body()->body();
    pmesh_body->applyCentralForce(
        Physics::toBulletVector(glm::vec3(0.0f, -31.0f * pmesh->mass(), 0.0f))
    );
    
    if(not m_bEnter){
        Sound::play(m_pCamera.get(), "spawn.wav", m_pQor->resources());
        m_bEnter = true;
    }

    if(dead())
    {
        if(not local()){
            if(not m_pNet->server())
                m_pSpec->respawn(this);
            return;
        }else{
        //}else if(
        //    m_pController->button("fire").pressed_now() ||
        //    m_pController->button("use").pressed_now()
        //){
            if(m_pNet->remote())
                m_pSpec->send_spawn(this);
            else
                m_pSpec->respawn(this);
            return;
        }
    }

    if(not local())
        return;
    
    if(m_LockIf && m_LockIf())
        return;

    if(m_pController->input()->key(SDLK_F2).pressed_now()) {
        m_pSpec->play(m_pProfile->session()->dummy_profile("Bot"));
    }
    
    //if(m_pController->input()->key(SDLK_z).pressed_now()) {
    //    hurt(1);
    //}
    if(m_pController->input()->key(SDLK_F1).pressed_now()){
        m_WeaponStash.give_all();
        refresh_weapon();
    }
    
    //if(m_pController->input()->key(SDLK_x).pressed_now()) {
    //    m_pState->event("message", make_shared<Meta>(MetaFormat::JSON,
    //        R"({"message": "RED TEAM SCORES", "color": "FF0000"})"
    //    ));
    //}
    //if(m_pController->input()->key(SDLK_c).pressed_now()) {
    //    m_pState->event("message", make_shared<Meta>(MetaFormat::JSON,
    //        R"({"message": "BLUE TEAM SCORES", "color": "0000FF"})"
    //    ));
    //}
    
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
        auto hits = m_pPhysics->hits(
            m_pCamera->position(Space::WORLD),
            m_pCamera->to_world(vec3(0.0f, 0.0f, -3.0f))
        );
        Node* n = nullptr;
        
        try{
            // ignore self
            n = std::get<0>(hits.at(0));
            if(n == m_pPlayerShape.get())
                n = std::get<0>(hits.at(1));
        }catch(std::out_of_range&){
        }
        if(n)
        {
            if(n->compositor())
                n = n->compositor();
            if(n->has_event("use")){
                Sound::play(m_pCamera.get(), "switch.wav", m_pQor->resources());
                if(not m_pNet->remote())
                    n->event("use", make_shared<Meta>());
            }
        }
    }


    for(int i=0; i<9; ++i)
    {
        if(not m_pViewModel->equipping()){
            if(m_pController->button(string("slot")+to_string(i))){
                if(slot(i))
                    break;
            }
        }
    }
    
    if(m_pController->button("next").pressed_now()){
        next_weapon();
    }
    
    if(m_pController->button("previous").pressed_now()){
        prev_weapon();
    }
    
    if(m_pController->button("reload").pressed_now())
    {
        reload();
    }
    
    if(m_pController->button("scores").pressed()) {
        m_pHUD->show_scores(true);
    }else{
        m_pHUD->show_scores(false);
    }

    if(m_pController->button("fire") &&
       m_pViewModel->idle() &&
       m_pViewModel->equipped()
    ){
        fire_weapon();
    }

    //LOGf("pos: %s, %s", t.s() % m_pPlayerShape->position().y);

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
}

void Player :: reload()
{
    auto _this = this;
    if(m_pViewModel->idle() && m_pViewModel->equipped()){
        //auto cache = m_pQor->resources();
        //auto camera = m_pCamera.get();
        if(m_WeaponStash.active()->can_reload()){
            on_event(PE_RELOAD);
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

void Player :: fire_weapon()
{

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
        int burst = m_WeaponStash.active()->fire();
        update_hud();
        if(burst)
        {
            on_event(PE_FIRE);
            //auto p = m_pQor->make<Particle>("muzzleflash1.png");
            //m_pViewModel->add(p);
            //p->move(vec3(0.0f, 0.0f, -0.4f));
            //p->scale(0.1f);
            //p->collapse(Space::WORLD);
            //p->life(Freq::Time::seconds(0.01f));
            
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
                    //if(not n->find("#player",Node::Find::REVERSE).empty())
                    if(n->compositor())
                        n = n->compositor();
                    
                    if(n->has_event("hit")){

                        if(not m_pNet->remote()){
                            auto hitinfo = make_shared<Meta>();
                            hitinfo->set<int>("damage", m_WeaponStash.active()->spec()->damage());
                            hitinfo->set<Player*>("owner", this);
                            n->event("hit", hitinfo);
                        }
                        
                        player_hit = true;
                        
                        auto blood = make_shared<Particle>("blood.png", m_pCache);
                        m_pRoot->add(blood);
                        blood->move(std::get<1>(hit));
                        blood->move(glm::normalize(
                            m_pPlayerShape->position(Space::WORLD) - blood->position(Space::WORLD)
                        ) * 0.25f);
                        blood->velocity(glm::vec3(
                            rand() % 100 / 100.0f,
                            rand() % 100 / 100.0f * 2.0f,
                            rand() % 100 / 100.0f));
                        blood->acceleration(glm::vec3(0.0f, -9.8f, 0.0f));
                        blood->scale(0.25f + rand()%10/10.0f*0.5f);
                        blood->life(Freq::Time::seconds(0.5f));
                    }
                    else
                    {
                        decal(n,
                            std::get<1>(hit),
                            std::get<2>(hit),
                            Matrix::up(*m_pCamera->matrix(Space::WORLD)),
                            i * 0.0001
                        );
                    }
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
            on_event(PE_FIRE);
            
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
            body->setCcdSweptSphereRadius(0.001f);
            auto vel = m->velocity();
            
            auto mp = m.get();
            auto cache = m_pCache;
            auto dmg = m_WeaponStash.active()->spec()->damage();
            auto radius = m_WeaponStash.active()->spec()->radius();
            auto _this = this;
            auto spec = m_pSpec;
            auto me = shape();
            
            auto splode = function<void()>([mp,dmg,cache,spec,_this,me,radius](){
                if(mp->detaching())
                    return;
                auto snd = make_shared<Sound>(
                    cache->transform("explosion.wav"), cache
                );
                auto particle = make_shared<Particle>("explosion.png", cache);
                particle->life(Freq::Time::seconds(0.2f));
                particle->offset(0.5f);
                auto p = particle.get();
                particle->on_tick.connect([p](Freq::Time t){
                    p->rescale(p->scale().x + p->scale().x * t.s());
                });
                mp->stick(particle);
                mp->stick(snd);
                snd->detach_on_done();
                snd->play();

                auto hitinfo = make_shared<Meta>();
                hitinfo->set<int>("damage", dmg);
                hitinfo->set<double>("radius", radius);
                hitinfo->set<Player*>("owner", _this);
                spec->splash(mp, hitinfo);
                
                mp->safe_detach();
            });
            
            m_pSpec->register_projectile(m, [_this,dmg,spec,splode,mp](Node* n,Node*){
                //auto player = (Player*)a->at<void*>("player");
                //auto hitinfo = make_shared<Meta>();
                //hitinfo->set<int>("damage", dmg);
                //hitinfo->set<double>("radius", 1.0f);
                //hitinfo->set<Player*>("owner", _this);
                ////n->event("hit", hitinfo);
                splode();
            });
            
            if(not m_WeaponStash.active()->spec()->gravity()) {
                body->setGravity(btVector3(0.0f, 0.0f, 0.0f));
                mp->inertia(false);
                //body->setAngularFactor(btVector3(0.0f, 1.0f, 0.0f));
                //m->on_tick.connect([mp,vel,cache](Freq::Time){
                //    auto body = ((btRigidBody*)mp->body()->body());
                //    if(glm::dot(mp->velocity(),vel) < 1.0f - K_EPSILON ||
                //        glm::length(mp->velocity()) != glm::length(vel))
                //    {
                //        auto snd = make_shared<Sound>(
                //            cache->transform("explosion.wav"), cache
                //        );
                //        mp->stick(snd);
                //        snd->detach_on_done();
                //        snd->play();
                //        mp->detach();
                //    }
                //});
                m_pPhysics->on_collision(mp, [splode](Node*,Node*,vec3,vec3,vec3){
                    splode();
                });
            }else{
                auto timer = make_shared<float>(1.0f);
                m->on_tick.connect([splode,mp,timer](Freq::Time t){
                    *timer -= t.s();
                    if(*timer <= 0.0f){
                        splode();
                        mp->detach();
                    }
                });
            }
            
            
        }else{
            Sound::play(m_pCamera.get(), "empty.wav", m_pQor->resources());
        }
    }
}

void Player :: decal(Node* n, glm::vec3 contact, glm::vec3 normal, glm::vec3 up, float offset)
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
    mat4 decal_space = glm::mat4(glm::orthonormalize(glm::mat3(
        right, up, normal
    )));
    *m->matrix() = decal_space;
    m->position(contact);
    m->move(normal * (0.001f + offset));
    m->pend();
    m_Decals.push_back(m);
    while(m_Decals.size() > MAX_DECALS) {
        auto f = m_Decals.front();
        f->detach();
        m_Decals.pop_front();
    }
    n->add(m);
    *m->matrix() = glm::inverse(*n->matrix(Space::WORLD)) * *m->matrix();
    m->pend();
    
    // spark
    auto m2 = make_shared<Mesh>(make_shared<MeshGeometry>(Prefab::quad(
        glm::vec2(decal_scale/2.0f, decal_scale/2.0f),
        glm::vec2(-decal_scale/2.0f, -decal_scale/2.0f)
    )));
    m2->add_modifier(make_shared<Wrap>(Prefab::quad_wrap()));
    m2->material(make_shared<MeshMaterial>(m_pSpark));
    *m2->matrix() = decal_space;
    m2->position(contact);
    m2->move(normal * (0.01f + offset));
    m2->pend();
    n->add(m2);
    *m2->matrix() = glm::inverse(*n->matrix(Space::WORLD)) * *m2->matrix();
    m2->pend();
    
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
    if(m_bCrouched)
        return false;
    auto pos = m_pPlayerShape->position(Space::WORLD);
    auto jump_hit = m_pPhysics->first_hit(
        pos,
        pos - glm::vec3(0.0f, 1.2f, 0.0f)
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
//    m_pPlayerShape->position(pos);
//}

void Player :: die()
{
    m_pViewModel->equip(false);
    m_pProfile->temp()->set<int>("hp", 0);
    m_pProfile->temp()->set<int>("deaths",
        1 + m_pProfile->temp()->at<int>("deaths")
    );
    Sound::play(m_pCamera.get(), "death.wav", m_pQor->resources());
    m_DeathMsg = m_pProfile->name() + " was fragged.";
    on_death();
}

void Player :: reset()
{
    //if(m_pSpec->respawn(this)){
    m_pViewModel->equip(true);
    
    m_pProfile->temp()->set<int>("maxhp", 10); // this won't trigger
    m_pProfile->temp()->set<int>("hp", 10); // ...so do this 2nd
    
    m_WeaponStash.clear();
    m_WeaponStash.give("glock");
    m_WeaponStash.give("shotgun");
    m_WeaponStash.slot(3);
    refresh_weapon();
    m_pPlayerShape->velocity(glm::vec3(0.0f));
    
    m_bEnter = false;
    //}
}

void Player :: hurt(int dmg)
{
    if(dead())
        return;

    on_hurt(dmg);
    
    int hp = m_pProfile->temp()->at<int>("hp");
    
    auto maxhp = m_pProfile->temp()->at<int>("maxhp");
    
    hp = std::min(std::max(0, hp - dmg), maxhp);
    m_pProfile->temp()->set<int>("hp", hp);

    if(not hp)
    {
        if(not m_pNet->remote())
            die();
    }
    else if(dmg > 0)
    {
        if(local()){
            m_FlashColor = Color::red();
            m_FlashAlarm.set(Freq::Time::seconds(2.0f * dmg*1.0f/10));
            if(m_HurtSoundAlarm.elapsed()){
                Sound::play(m_pCamera.get(), "hurt.wav", m_pQor->resources());
                m_HurtSoundAlarm.set(Freq::Time::seconds(0.5f));
            }
        }
        else{
            if(m_HurtSoundAlarm.elapsed()){
                Sound::play(m_pCamera.get(), "grunt.wav", m_pQor->resources());
                m_HurtSoundAlarm.set(Freq::Time::seconds(0.5f));
            }
        }
    }
    else if(dmg < 0)
    {
        if(local()){
            m_FlashColor = Color::green();
            m_FlashAlarm.set(Freq::Time::seconds(0.5f));
            Sound::play(m_pCamera.get(), "health.wav", m_pQor->resources());
        }
        else
            Sound::play(m_pCamera.get(), "health.wav", m_pQor->resources());
    }
    
    update_hud();
}

bool Player :: alive()
{
    int hp = m_pProfile->temp()->at<int>("hp");
    assert(hp >= 0);
    return hp > 0;
}
    
bool Player :: dead()
{
    int hp = m_pProfile->temp()->at<int>("hp");
    assert(hp >= 0);
    return hp == 0;
}
        
void Player :: give(const shared_ptr<Meta>& item)
{
    auto name = item->at<string>("name", "");
    if(name.empty())
        return;

    on_give(item->at<string>("name"));
    
    std::string cur = m_WeaponStash.active()->spec()->name();
    if(m_WeaponStash.give(item)){
        if(not local())
            return;
        auto item_name = item->at<string>("name");
        if(weapon_priority_cmp(item_name, cur)){
            m_WeaponStash.slot(item_name);
            refresh_weapon();
        }
        LOGf("Picked up %s!",
            m_pSpec->config()->meta("weapons")->meta(name)->template at<string>("name", "???")
        );
        update_hud();
        m_FlashColor = Color::yellow();
        m_FlashAlarm.set(Freq::Time::seconds(0.5f));
        Sound::play(m_pCamera.get(), "reload.wav", m_pCache);
        return;
    } else if(name == "medkit") {
        if(not local())
            return;
        LOGf("Picked up %s!", 
            m_pSpec->config()->meta("items")->meta(name)->template at<string>("name", "???")
        );
        heal(10);
    } else if (name == "ammobox") {
        if(not local())
            return;
        LOGf("Picked up %s!",
            m_pSpec->config()->meta("items")->meta(name)->template at<string>("name", "???")
        );

        m_FlashColor = Color::yellow();
        m_FlashAlarm.set(Freq::Time::seconds(0.5f));
        Sound::play(m_pCamera.get(), "reload.wav", m_pCache);
        m_WeaponStash.fill_all();
    } else if (name == "redflag") {
        Sound::play(m_pCamera.get(), "redflagtaken.wav", m_pCache);
        m_pState->event("message", make_shared<Meta>(MetaFormat::JSON,
            R"({"message": "RED FLAG TAKEN", "color": "FF0000"})"
        ));
        //m_pHUD->red_pulse(true);
    } else if (name == "blueflag") {
        Sound::play(m_pCamera.get(), "blueflagtaken.wav", m_pCache);
        m_pState->event("message", make_shared<Meta>(MetaFormat::JSON,
            R"({"message": "BLUE FLAG TAKEN", "color": "0000FF"})"
        ));
        //m_pHUD->blue_pulse(true);
    }


    // Item is something else...
}

void Player :: give(std::string what)
{
    auto meta = make_shared<Meta>();
    meta->set<string>("name", what);
    give(meta);
}
    
void Player :: add_frag(Player* target)
{
    string msg;
    int frags = m_pProfile->temp()->at<int>("frags");
    if(target == this){
        // suicide
        m_pProfile->temp()->set<int>("frags", frags - 1);
        m_pHUD->frags(frags - 1);
        msg = name() + " suicided!";
    }else{
        // normal frag
        m_pProfile->temp()->set<int>("frags", frags + 1);
        m_pHUD->frags(frags + 1);
        msg = (boost::format("%s fragged %s.") % name() % target->name()).str();
        m_pState->event("message", make_shared<Meta>(MetaFormat::JSON,
            R"({"message": "YOU FRAGGED )" +
                boost::to_upper_copy(target->name()) +
                R"(", "color": "FFFFFF"})"
        ));
    }
    if(not m_pNet->remote()){ // server or local player
        LOG(msg);
    }else{
        m_pNet->message(msg);
    }
    
    on_frag(target);
}

bool Player :: weapon_priority_cmp(string s1, string s2)
{
    for(auto&& e: *m_pProfile->config()->meta("weapon_priority"))
    {
        string s = e.as<string>();
        if(s == s1)
            return true;
        if(s == s2)
            return false;
    }
    return false;
}

void Player :: unpack_transform(mat4 m)
{
    //LOG(Matrix::to_string(m));
    auto r = glm::extractMatrixRotation(m);
    auto p = m_pCamera->position();
    m_pCamera->set_matrix(r);
    m_pCamera->position(p);
    if(m_pPlayerModel)
        m_pPlayerModel->set_matrix(r);
    m_pPlayerShape->teleport(glm::translate(glm::mat4(), Matrix::translation(m)));
    m_pPlayerShape->pend();
}

glm::mat4 Player :: pack_transform()
{
    mat4 r;
    if(local())
        r = *m_pCamera->matrix(Space::WORLD);
    else
        r = *m_pPlayerModel->matrix(Space::WORLD);
    
    //if(glm::extractMatrixRotation(r) != glm::mat4(1.0f)){
    //}else{
    //    static int i = 0;
    //    if(++i > 100)
    //        assert(false);
    //}
    Matrix::translation(r, m_pPlayerShape->position());
    return r;
}

void Player :: next_weapon()
{
    auto _this = this;
    if(m_pViewModel->idle()){
        if(m_WeaponStash.next()){
            on_event(PE_NEXT);
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

void Player :: prev_weapon()
{
    auto _this = this;
    if(m_pViewModel->idle()){
        if(m_WeaponStash.next(-1)){
            on_event(PE_PREV);
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

void Player :: do_event(unsigned char ev)
{
    if(ev == PE_FIRE)
        fire_weapon();
    else if(ev == PE_RELOAD)
        reload();
    else if(ev == PE_NEXT)
        next_weapon();
    else if(ev == PE_PREV)
        prev_weapon();
    else if(ev == PE_DIE)
        die();
    //else if(ev == PE_FRAG)
    //    add_frags(1);
}

bool Player :: slot(unsigned i)
{
    auto _this = this;
    if(m_WeaponStash.slot(i)){
        on_slot(i);
        
        Sound::play(m_pCamera.get(), "swap.wav", m_pQor->resources());
        scope(false);
        m_pViewModel->fast_zoom(false);
        auto vm = m_pViewModel.get();
        m_pViewModel->equip(false,[_this]{
            _this->refresh_weapon();
        });
        return true;
    }
    return false;
}

void Player :: knockback()
{
    m_KnockbackAlarm.set(Freq::Time::seconds(0.5f));
}


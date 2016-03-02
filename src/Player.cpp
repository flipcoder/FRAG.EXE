#include "Player.h"
#include "Qor/Sound.h"
#include "Qor/Qor.h"
#include "Qor/Physics.h"
using namespace std;
using namespace glm;

const unsigned Player :: MAX_DECALS = 32;

Player :: Player(
    Node* root,
    shared_ptr<Controller> controller,
    Cache<Resource, string>* cache,
    Physics* physics,
    Window* window,
    Qor* engine,
    GameSpec* spec,
    std::function<bool()> lock_if
):
    m_pRoot(root),
    m_pController(controller),
    m_pCache(cache),
    m_pPhysics(physics),
    m_pWindow(window),
    m_pQor(engine),
    m_pGameSpec(spec),
    m_WeaponStash(spec->weapons())
{
    m_pPlayerMesh = make_shared<Mesh>();
    m_pPlayerMesh->set_box(Box(
        vec3(-0.2f, -0.6f, -0.2f),
        vec3(0.2f, 0.6f, 0.2f)
    ));
    m_pPlayerMesh->disable_physics();
    m_pPlayerMesh->set_physics(Node::Physics::DYNAMIC);
    m_pPlayerMesh->set_physics_shape(Node::CAPSULE);
    m_pPlayerMesh->friction(0.0f);
    m_pPlayerMesh->mass(80.0f);
    m_pPlayerMesh->inertia(false);
    m_pCamera = make_shared<Camera>(cache, window);
    //m_pRoot->add(m_pCamera);
    m_pPlayerMesh->add(m_pCamera);
    m_pCamera->position(vec3(0.0f, 0.6f, 0.0f));
    m_pRoot->add(m_pPlayerMesh);
    m_pDecal = cache->cache_cast<ITexture>("decal_bullethole1.png");
    m_pSpark = cache->cache_cast<ITexture>("spark.png");
    m_pController = m_pQor->session()->profile(0)->controller();

    m_WeaponStash.give_all();
    m_WeaponStash.next();
    refresh_weapon();
    
    m_pInterface = kit::init_shared<PlayerInterface3D>(
        m_pController,
        m_pCamera,
        m_pPlayerMesh,
        m_pQor->session()->profile(0)->config(),
        m_LockIf
    );
    m_pInterface->speed(12.0f);
    
    //btRigidBody* pmesh_body = (btRigidBody*)m_pPlayerMesh->body()->body();
    auto pmesh = m_pPlayerMesh.get();
    auto camera = m_pCamera.get();
    auto interface = m_pInterface.get();
    auto _this = this;
    m_pPhysics->on_generate([_this, physics, pmesh,interface,cache,camera]{
        auto pmesh_body = (btRigidBody*)pmesh->body()->body();
        pmesh_body->setActivationState(DISABLE_DEACTIVATION);
        pmesh_body->setAngularFactor(btVector3(0,0,0));
        pmesh_body->setCcdMotionThreshold(1.0f);
        //pmesh_body->setCcdSweptSphereRadius(0.25f);
        //pmesh_body->setRestitution(0.0f);
        //pmesh_body->setDamping(0.0f, 0.0f);
        ////pmesh_body->setRestitution(0.0f);
        interface->on_jump([_this, physics, pmesh_body, cache, camera]{
            if(not _this->can_jump())
                return;
            pmesh_body->applyImpulse(
                btVector3(0.0f, 1000.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)
            );
            Sound::play(camera, "jump.wav", cache);
        });
    });
    m_pPlayerMesh->move(vec3(0.0f, 0.6f, 0.0f));

    m_pCamera->perspective();
    m_pCamera->listen();
    
    //m_pInterface->fly();
    //btRigidBody* pmesh_body = (btRigidBody*)m_pPlayerMesh->body()->body();
    
}

void Player :: logic(Freq::Time t)
{
    if(m_LockIf && m_LockIf())
        return;
    
    if(m_pController->button("zoom").pressed_now())
        m_pViewModel->zoom(not m_pViewModel->zoomed());

    for(int i=0; i<9; ++i)
    {
        if(not m_pViewModel->equipping()){
            if(m_pController->button(string("slot")+to_string(i))){
                if(m_WeaponStash.slot(i)){
                    m_pViewModel->fast_zoom(false);
                    auto vm = m_pViewModel.get();
                    auto _this = this;
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
                m_pViewModel->fast_zoom(false);
                auto vm = m_pViewModel.get();
                auto _this = this;
                m_pViewModel->equip(false,[_this]{
                    _this->refresh_weapon();
                });
            }
        }
    }
    
    if(m_pController->button("previous").pressed_now()){
        if(m_pViewModel->idle()){
            if(m_WeaponStash.next(-1)){
                m_pViewModel->fast_zoom(false);
                auto vm = m_pViewModel.get();
                auto _this = this;
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
            m_pViewModel->fast_zoom(false);
            auto vm = m_pViewModel.get();
            m_pViewModel->equip_time(Freq::Time(250));
            Sound::play(m_pCamera.get(), "reload.wav", m_pQor->resources());
            m_pViewModel->equip(false, [vm]{
                vm->equip(true);
            });
        }
    }

    if(m_pController->button("fire") &&
       m_pViewModel->idle() &&
       m_pViewModel->equipped()
    ){
        //Sound::play(m_pCamera.get(), "shotgun.wav", m_pQor->resources());
        Sound::play(m_pCamera.get(), m_WeaponStash.active()->spec()->sound(), m_pQor->resources());
        
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
        m_pViewModel->recoil(Freq::Time(50), m_WeaponStash.active()->spec()->delay(), 0.05f);

        bool player_hit = false;
        if(m_WeaponStash.active()->spec()->projectile().empty())
        {
            for(int i=0; i<m_WeaponStash.active()->spec()->burst(); ++i)
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
                    if(n->parent()->has_tag("player"))
                        player_hit = true;
                    
                    decal(
                        std::get<1>(hit),
                        std::get<2>(hit),
                        Matrix::up(*m_pCamera->matrix(Space::WORLD)),
                        i * 0.0001
                    );
                }
            }
            if(player_hit)
                Sound::play(m_pCamera.get(), "hit.wav", m_pQor->resources());
        }
        else
        {
            // projectile
            auto m = m_pQor->make<Mesh>("projectile_grenade.obj");
            //m->set_physics(Node::DYNAMIC);
            //m->set_physics_shape(Node::HULL);
            //m->mass(1.0f);
            //m_pPhysics->generate(m.get());
            m_pViewModel->add(m);
            m->collapse();
        }
        
    }

    //LOGf("pos: %s, %s", t.s() % m_pPlayerMesh->position().y);

    //LOGf("children: %s", m_pRoot->num_subnodes());

    m_pViewModel->sway(m_pInterface->move() != glm::vec3(0.0f));
    m_pViewModel->sprint(
        m_pInterface->move() != glm::vec3(0.0f) && m_pInterface->sprint()
    );
    
    //m_pViewModel->position(m_pCamera->position(Space::WORLD));

    auto input = m_pController->input();
    if(input->key(SDLK_DOWN))
        m_pViewModel->zoomed_model_move(glm::vec3(0.0f, -t.s() * 0.1f, 0.0f));
    else if(input->key(SDLK_UP))
        m_pViewModel->zoomed_model_move(glm::vec3(0.0f, t.s() * 0.1f, 0.0f));
    else if(input->key(SDLK_LEFT))
        m_pViewModel->zoomed_model_move(glm::vec3(-t.s() * 0.1f, 0.0f, 0.0f));
    else if(input->key(SDLK_RIGHT))
        m_pViewModel->zoomed_model_move(glm::vec3(t.s() * 0.1f, 0.0f, 0.0f));
    else if(input->key(SDLK_w))
        m_pViewModel->zoomed_model_move(glm::vec3(0.0f, 0.0f, t.s() * 0.1f));
    else if(input->key(SDLK_r))
        m_pViewModel->zoomed_model_move(glm::vec3(0.0f, 0.0f, -t.s() * 0.1f));

    LOGf("model pos %s", Vector::to_string(m_pViewModel->model_pos()));
    LOGf("zoomed model pos %s", Vector::to_string(m_pViewModel->zoomed_model_pos()));
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
        m_pViewModel->model_pos(wpn->viewmodel_pos());
        m_pViewModel->zoomed_model_pos(wpn->viewmodel_zoomed_pos());
        m_pViewModel->reset_zoom();
        m_pViewModel->fast_equip(false);
        m_pViewModel->equip(true);
    }
}


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
    std::function<bool()> lock_if
):
    m_pRoot(root),
    m_pController(controller),
    m_pCache(cache),
    m_pPhysics(physics),
    m_pWindow(window),
    m_pQor(engine)
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
    m_pDecal = cache->cache_cast<ITexture>(
        "decal_bullethole1.png"
    );
    m_pController = m_pQor->session()->profile(0)->controller();
    
    const bool ads = false;
    //auto gun = m_pQor->make<Mesh>("gun_tacticalsupershotgun.obj");
    //auto gun = m_pQor->make<Mesh>("gun_bullpup.obj");
    auto gun = m_pQor->make<Mesh>("ump45.obj");
    //auto gun = m_pQor->make<Mesh>("glock.obj");
    m_pViewModel = make_shared<ViewModel>(m_pCamera, gun);
    gun->disable_physics();
    m_pRoot->add(m_pViewModel);
    
    m_pViewModel->model_pos(glm::vec3(
        //0.111f, -0.168f, -0.223f //glock
        //0.0f, -0.228f, -0.385f //shotgun
        0.093, -0.288f, -0.32f //ump45
    ));
    m_pViewModel->zoomed_model_pos(glm::vec3(
        //0.0f, -0.039f, -0.282f // glock
        //0.0f, -0.15f, -0.472f // shotgun
        0.0f, -0.211f, -0.407 //ump45
    ));
    m_pViewModel->reset_zoom();
    
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
    m_pPhysics->on_generate([pmesh,interface,cache,camera]{
        auto pmesh_body = (btRigidBody*)pmesh->body()->body();
        pmesh_body->setActivationState(DISABLE_DEACTIVATION);
        pmesh_body->setAngularFactor(btVector3(0,0,0));
        pmesh_body->setCcdMotionThreshold(1.0f);
        //pmesh_body->setCcdSweptSphereRadius(0.25f);
        //pmesh_body->setRestitution(0.0f);
        //pmesh_body->setDamping(0.0f, 0.0f);
        ////pmesh_body->setRestitution(0.0f);
        interface->on_jump([pmesh_body, cache, camera]{
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
    
    //if(m_pController->button("next").pressed_now() ||
    //   m_pController->button("previous").pressed_now())
    if(m_pController->button("reload").pressed_now())
    {
        //auto cache = m_pQor->resources();
        //auto camera = m_pCamera.get();
        auto vm = m_pViewModel.get();
        m_pViewModel->equip_time(Freq::Time(250));
        Sound::play(m_pCamera.get(), "reload.wav", m_pQor->resources());
        m_pViewModel->equip(false, [vm]{
            vm->equip(true);
        });
    }

    if(m_pController->button("fire") &&
       m_pViewModel->idle() &&
       m_pViewModel->equipped()
    ){
        //Sound::play(m_pCamera.get(), "shotgun.wav", m_pQor->resources());
        Sound::play(m_pCamera.get(), "shot.wav", m_pQor->resources());
        
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
        //m_pViewModel->recoil(Freq::Time(50), Freq::Time(700)); // shotgun
        m_pViewModel->recoil(Freq::Time(25), Freq::Time(50), 0.05f); // ump45

        //for(int i=0; i<8; ++i)
        int i = 0;
        {
            auto mag_var = (rand() % 1000) * 0.001f * 0.1f         * 0.0f; // ump has 0
            auto ang_var = (rand() % 1000) * 0.001f                * 0.0f;
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
            if(std::get<0>(hit))
            {
                decal(
                    std::get<1>(hit),
                    std::get<2>(hit),
                    Matrix::up(*m_pCamera->matrix(Space::WORLD)),
                    i * 0.0001
                );
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

    //if(m_pInput->key(SDLK_DOWN))
    //    m_pViewModel->zoomed_model_move(glm::vec3(0.0f, -t.s(), 0.0f));
    //else if(m_pInput->key(SDLK_UP))
    //    m_pViewModel->zoomed_model_move(glm::vec3(0.0f, t.s(), 0.0f));
    //else if(m_pInput->key(SDLK_LEFT))
    //    m_pViewModel->zoomed_model_move(glm::vec3(-t.s(), 0.0f, 0.0f));
    //else if(m_pInput->key(SDLK_RIGHT))
    //    m_pViewModel->zoomed_model_move(glm::vec3(t.s(), 0.0f, 0.0f));
    //else if(m_pInput->key(SDLK_w))
    //    m_pViewModel->zoomed_model_move(glm::vec3(0.0f, 0.0f, t.s()));
    //else if(m_pInput->key(SDLK_r))
    //    m_pViewModel->zoomed_model_move(glm::vec3(0.0f, 0.0f, -t.s()));

    //LOGf("model pos %s", Vector::to_string(m_pViewModel->zoomed_model_pos()));
    //LOGf("zoomed model pos %s", Vector::to_string(m_pViewModel->zoomed_model_pos()));
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
}


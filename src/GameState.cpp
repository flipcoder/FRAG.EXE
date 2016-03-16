#include "GameState.h"
#include "Qor/Input.h"
#include "Qor/Qor.h"
#include "Qor/TileMap.h"
#include "Qor/ScreenFader.h"
#include "Qor/Sound.h"
#include "Qor/Sprite.h"
#include "Qor/Particle.h"
#include <glm/glm.hpp>
#include <cstdlib>
#include <chrono>
#include <thread>
#include "Qor/PlayerInterface3D.h"
#include "Qor/Light.h"
#include "Qor/Material.h"
#include "Qor/kit/log/log.h"
#include <glm/gtx/orthonormalize.hpp>
using namespace std;
using namespace glm;

GameState :: GameState(
    Qor* engine
    //std::string fn
):
    m_pQor(engine),
    m_pInput(engine->input()),
    m_pRoot(make_shared<Node>()),
    m_pSkyboxRoot(make_shared<Node>()),
    m_pInterpreter(engine->interpreter()),
    m_pScript(make_shared<Interpreter::Context>(engine->interpreter())),
    m_pPipeline(engine->pipeline()),
    m_GameSpec("game.json", engine->resources())
{
    m_Shader = m_pPipeline->load_shaders({"lit"});
}

void GameState :: preload()
{
    m_pPhysics = make_shared<Physics>(m_pRoot.get(), this);
    auto win = m_pQor->window();
    m_pController = m_pQor->session()->profile(0)->controller();
    m_pPlayer = kit::make_unique<Player>(
        m_pRoot.get(),
        m_pController,
        m_pQor->resources(),
        m_pPhysics.get(),
        win,
        m_pQor,
        &m_GameSpec,
        [&]{ return m_pConsole->input();}
    );
    
    // just render this player for now
    m_pCamera = m_pPlayer->camera();
    m_pOrthoCamera = m_pPlayer->ortho_camera();
    m_pOrthoRoot = m_pPlayer->ortho_root();
    
    m_pConsoleCamera = make_shared<Camera>(m_pQor->resources(), m_pQor->window());
    m_pConsoleRoot = make_shared<Node>();

    //auto l = make_shared<Light>();
    //l->dist(20.0f);
    //l->position(glm::vec3(0.0f, 1.0f, 0.0f));
    //m_pRoot->add(l);

    //auto p = m_pQor->make<Particle>("particle.png");
    //p->position(vec3(-1.0f, 1.0f, 0.0f));
    //p->flags(Particle::UPRIGHT);
    //m_pRoot->add(p);
    
    //p = m_pQor->make<Particle>("particle.png");
    //p->position(vec3(1.0f, 1.0f, 0.0f));
    //p->scale(0.5f);
    //p->flags(Particle::UPRIGHT);
    //m_pRoot->add(p);
    
    //auto player = m_pQor->make<Mesh>("player.obj");
    //player->position(vec3(0.0f, 1.0f, 0.0f));
    //auto playerptr = player.get();
    //player->config()->set<int>("hp", 10);
    //player->event("hit", [playerptr](const shared_ptr<Meta>& meta){
    //    LOG("hit!");
    //    int dmg = meta->at<int>("damage");
    //    int hp = playerptr->config()->at<int>("hp");
    //    if(dmg && hp){
    //        hp = std::max<int>(0, hp - dmg);
    //        playerptr->config()->set<int>("hp",hp);
    //        if(hp <= 0)
    //            playerptr->detach();
    //    }
    //});
    //m_pRoot->add(player);
    
    auto mus = m_pQor->make<Sound>("cave.ogg");
    m_pRoot->add(mus);
    mus->ambient(true);
    
    m_pSkyboxCamera = make_shared<Camera>(m_pQor->resources(), m_pQor->window());
    m_pSkyboxCamera->perspective();
    m_pSkyboxRoot->add(m_pQor->make<Mesh>("skybox1.obj"));
    m_pSkyboxCamera->track(m_pCamera);
    m_pSkyboxCamera->mode(Tracker::ORIENT);
    m_pSkyboxRoot->add(m_pSkyboxCamera);
    
    //m_pPipeline = make_shared<Pipeline>(
    //    m_pQor->window(),
    //    m_pQor->resources(),
    //    m_pRoot,
    //    m_pCamera
    //);
    
    //m_pRoot->add(m_pQor->make<Mesh>("apartment_scene.obj"));
    string map = m_pQor->args().filenames(-1, "test.json");
    if(Filesystem::getExtension(map) == "json"){
        auto scene = m_pQor->make<Scene>(map);
        m_pRoot->add(scene->root());
    }else{
        m_pRoot->add(m_pQor->make<Mesh>(map));
    }
    
    m_pPhysics->generate(m_pRoot.get(), Physics::GEN_RECURSIVE);
    m_pPhysics->world()->setGravity(btVector3(0.0, -9.8, 0.0));

    m_pConsole = make_shared<Console>(m_pQor->interpreter(), win, m_pInput, m_pQor->resources());
    m_pConsoleRoot->add(m_pConsole);
    
    map = Filesystem::cutExtension(map);
    // TODO: ensure filename contains only valid filename chars
    if(not map.empty())
        m_pScript->execute_file("mods/FRAG.EXE/data/maps/"+ map +".py");

    //auto lights = m_pRoot->hook_type<Light>();
    //for(auto&& l: lights)
    //    l->detach();
}

GameState :: ~GameState()
{
}

void GameState :: enter()
{
    m_pRoot->each([](Node* node){
        auto s = dynamic_cast<Sound*>(node);
        if(s) {
            s->play();
        }
    }, Node::Each::RECURSIVE);

    auto spawns = m_pRoot->hook(R"(Spawn\.*)", Node::Hook::REGEX);
    if(not spawns.empty())
    {
        auto spawn = spawns[rand() % spawns.size()];
        m_pPlayer->mesh()->teleport(spawn->position(Space::WORLD));
    }

    //m_pPlayer = kit::init_shared<PlayerInterface3D>(
    //    m_pController,
    //    m_pCamera,
    //    m_pPlayerMesh,
    //    m_pQor->session()->profile(0)->config(),
    //    [&]{ return m_pConsole->input(); }
    //);
    //m_pPlayer->speed(12.0f);
    ////m_pPlayer->fly();
    //btRigidBody* pmesh_body = (btRigidBody*)m_pPlayerMesh->body()->body();
    //{
    //    auto resources = m_pQor->resources();
    //    auto camera = m_pCamera.get();
    //    m_pPlayer->on_jump([pmesh_body, resources, camera]{
    //        pmesh_body->applyImpulse(
    //            btVector3(0.0f, 1000.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)
    //        );
    //        Sound::play(camera, "jump.wav", resources);
    //    });
    //}
    Audio::reference_distance(2.0f);
    Audio::max_distance(20.0f);
    
    m_pPipeline->shader(1)->use();
    m_pPipeline->override_shader(PassType::NORMAL, m_Shader);
     
    //m_pCamera->perspective();
    //m_pCamera->listen();
    m_pInput->relative_mouse(true);

    on_tick.connect(std::move(screen_fader(
        [this](Freq::Time, float fade) {
            m_pPipeline->shader(1)->use();
            int fadev = m_pPipeline->shader(1)->uniform("Brightness");
            if(fadev != -1)
                m_pPipeline->shader(1)->uniform(
                    fadev,
                    glm::vec3(fade,fade,fade)
                );
        },
        [this](Freq::Time){
            if(not m_pConsole->input() && m_pInput->escape())
                return true;
            return false;
        },
        [this](Freq::Time){
            m_pPipeline->shader(1)->use();
            int u = m_pPipeline->shader(1)->uniform("Brightness");
            if(u >= 0)
                m_pPipeline->shader(1)->uniform(u, Color::white().vec3());
            m_pPipeline->blend(false);
            m_pQor->pop_state();
        }
    )));
}

void GameState :: logic(Freq::Time t)
{
    Actuation::logic(t);
    m_pPhysics->logic(t);
    
    m_pPlayer->logic(t);
    //LOG(Vector::to_string(m_pPlayer->mesh()->position(Space::WORLD)));

    //auto box = m_pQor->make<Mesh>("box.obj");
    //box->set_physics_shape(Node::HULL);
    //auto shape = m_pPhysics->generate_shape(box.get());
    //auto body = kit::make_unique<btRigidBody>(info);
    //m_pPhysics->contact((btRigidBody*)body.get());
    
    m_pSkyboxRoot->logic(t);
    m_pOrthoRoot->logic(t);
    m_pRoot->logic(t);
}

void GameState :: render() const
{
    // render player view & skybox
    m_pPipeline->override_shader(PassType::NORMAL, (unsigned)PassType::NONE);
    m_pPipeline->winding(false);
    m_pPipeline->blend(false);
    m_pPipeline->render(
        m_pSkyboxRoot.get(),
        m_pSkyboxCamera.get(),
        nullptr,
        Pipeline::NO_DEPTH
    );
    m_pPipeline->override_shader(PassType::NORMAL, m_Shader);
    m_pPipeline->winding(false);
    m_pPipeline->blend(false);
    m_pPipeline->render(
        m_pRoot.get(),
        m_pCamera.get(),
        nullptr,
        Pipeline::LIGHTS | Pipeline::NO_CLEAR
    );

    // render ortho overlays
    m_pPipeline->override_shader(PassType::NORMAL, (unsigned)PassType::NONE);
    m_pPipeline->winding(true);
    m_pPipeline->render(
        m_pOrthoRoot.get(),
        m_pOrthoCamera.get(),
        nullptr,
        Pipeline::NO_CLEAR | Pipeline::NO_DEPTH
    );

    // render console
    m_pPipeline->render(
        m_pConsoleRoot.get(),
        m_pConsoleCamera.get(),
        nullptr,
        Pipeline::NO_CLEAR | Pipeline::NO_DEPTH
    );
    m_pPipeline->blend(false);
}


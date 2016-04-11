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
#include "Qor/BasicPartitioner.h"
using namespace std;
using namespace glm;

GameState :: GameState(
    Qor* engine
    //std::string fn
):
    m_pQor(engine),
    m_pInput(engine->input()),
    m_pPartitioner(engine->pipeline()->partitioner()),
    m_pRoot(make_shared<Node>()),
    m_pSkyboxRoot(make_shared<Node>()),
    m_pInterpreter(engine->interpreter()),
    m_pScript(make_shared<Interpreter::Context>(engine->interpreter())),
    m_pPipeline(engine->pipeline()),
    m_GameSpec(
        "game.json", engine->resources(), m_pRoot.get(), m_pPartitioner,
        engine->session()->profile(0)->controller(), engine,
        this
    )
{
    m_Shader = m_pPipeline->load_shaders({"lit"});

    if(m_pQor->args().has('d', "dedicated"))
        m_bDedicated = true;
}

void GameState :: preload()
{
    auto _this = this;
    
    m_pPhysics = make_shared<Physics>(m_pRoot.get(), this);
    m_GameSpec.set_physics(m_pPhysics.get());
    m_GameSpec.on_player_spawn.connect([_this](Player* p){
        if(p->local())
        {
            _this->m_pSkyboxRoot->add(_this->m_pSkyboxCamera);
            _this->m_pSkyboxCamera->track(p->camera());
            _this->m_pSkyboxCamera->position(glm::vec3(0.0f));
        }
    });
    m_GameSpec.on_spectator_spawn.connect([_this](Spectator* s){
        _this->m_pSkyboxRoot->add(_this->m_pSkyboxCamera);
        _this->m_pSkyboxCamera->track(s->camera());
        _this->m_pSkyboxCamera->position(glm::vec3(0.0f));
    });
     
    m_pConsoleCamera = make_shared<Camera>(m_pQor->resources(), m_pQor->window());
    m_pConsoleRoot = make_shared<Node>();
    m_pConsoleCamera->ortho(false);
    m_pConsole = make_shared<Console>(
        m_pQor->interpreter(), m_pQor->window(), m_pInput, m_pQor->resources()
    );
    m_pConsoleRoot->add(m_pConsole);
    auto console = m_pConsole.get();
    m_GameSpec.set_lock([console]{ return console->input(); });

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
    
    m_pSkyboxCamera = make_shared<Camera>(m_pQor->resources(), m_pQor->window());
    m_pSkyboxCamera->perspective();
    m_pSkyboxRoot->add(m_pQor->make<Mesh>("skybox2.obj"));
    m_pSkyboxRoot->add(m_pSkyboxCamera);
    m_pSkyboxCamera->mode(Tracker::ORIENT);
    
    //m_pPipeline = make_shared<Pipeline>(
    //    m_pQor->window(),
    //    m_pQor->resources(),
    //    m_pRoot,
    //    m_pCamera
    //);
    
    //m_pRoot->add(m_pQor->make<Mesh>("apartment_scene.obj"));
    string map = m_pQor->args().filenames(-1, "test");
    std::shared_ptr<Node> scene_root;
    
    //if(Filesystem::getExtension(map) == "json"){
    if(m_pQor->exists(map +  ".json")){
        auto scene = m_pQor->make<Scene>(map + ".json");
        auto scene_root = scene->root();
        m_pRoot->add(scene->root());
        auto meshes = scene_root->hook_type<Mesh>();
        for(auto&& mesh: meshes)
            mesh->set_physics(Node::STATIC);
    }
    
    if(m_pQor->exists(map +  ".obj")){
        auto scene_root = m_pQor->make<Mesh>(map + ".obj");
        m_pRoot->add(scene_root);
        auto meshes = scene_root->hook_type<Mesh>();
        for(auto&& mesh: meshes) {
            auto meshparent = mesh->compositor() ? mesh->compositor()->parent() : mesh->parent();
            if(not dynamic_cast<Particle*>(meshparent))
                mesh->set_physics(Node::STATIC);

            if(mesh->material()){
                if(Filesystem::getFileName(mesh->material()->texture()->filename()) == "caulk.png")
                    mesh->detach();
            }
        }
    }
    
    m_pPhysics->generate(m_pRoot.get(), Physics::GEN_RECURSIVE);
    m_pPhysics->world()->setGravity(btVector3(0.0, -9.8, 0.0));

    //auto lights = m_pRoot->hook_type<Light>();
    //for(auto&& l: lights)
    //    l->detach();
    
    // TODO: ensure filename contains only valid filename chars
    if(not map.empty())
        m_pScript->execute_file("mods/FRAG.EXE/data/maps/"+ map +".py");

    m_GameSpec.setup();

    // cache
    m_pQor->make<Mesh>("player.obj");
}

GameState :: ~GameState()
{
}

void GameState :: enter()
{
    if(not m_bDedicated)
        m_GameSpec.spawn_local_spectator();

    //m_GameSpec.play(nullptr);
    //m_GameSpec.play(nullptr);
    
    //m_pRoot->each([](Node* node){
    //    auto s = dynamic_cast<Sound*>(node);
    //    if(s) {
    //        s->play();
    //    }
    //}, Node::Each::RECURSIVE);

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

    auto cache = m_pQor->resources();
    auto root = m_pRoot;
    event("message", [root, cache](const std::shared_ptr<Meta>& m){
        string msg = m->at<string>("message");
        auto fn = boost::to_lower_copy(boost::replace_all_copy(msg, " ", ""))+".wav";
        auto snd = make_shared<Sound>(cache->transform(fn), cache);
        snd->ambient(true);
        root->add(snd);
        snd->play();
    });
    //event("message", make_shared<Meta>(
    //    MetaFormat::JSON, R"({"message": "RED TEAM SCORES", "color": "FF0000"})"
    //));
}

void GameState :: logic(Freq::Time t)
{
    t = m_GameTime.logic(t);
    
    Actuation::logic(t);
    m_pPhysics->logic(t);
    
    //if(m_pPlayer)
    //    m_pPlayer->logic(t);
    //if(m_pSpectator)
    //    m_pSpectator->logic(t);
    
    m_pConsoleRoot->logic(t);
    m_pSkyboxRoot->logic(t);
    m_pRoot->logic(t);
    m_GameSpec.logic(t);
    
    //LOGf("skybox: %s", Vector::to_string(m_pSkyboxCamera->position(Space::WORLD)));
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

    auto camera = m_GameSpec.camera();
    if(camera)
    {
        m_pPipeline->override_shader(PassType::NORMAL, m_Shader);
        m_pPipeline->winding(false);
        m_pPipeline->blend(false);
        m_pPipeline->render(
            m_pRoot.get(),
            camera.get(),
            nullptr,
            Pipeline::LIGHTS | Pipeline::NO_CLEAR
        );
    }

    // render ortho overlays
    m_pPipeline->override_shader(PassType::NORMAL, (unsigned)PassType::NONE);
    m_pPipeline->winding(true);
    m_pPipeline->render(
        m_GameSpec.ortho_root().get(),
        m_GameSpec.ortho_camera().get(),
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


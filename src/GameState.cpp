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
//#include <OALWrapper/OAL_Funcs.h>
using namespace std;
using namespace glm;

GameState :: GameState(
    Qor* engine
    //std::string fn
):
    m_pQor(engine),
    m_pInput(engine->input()),
    m_pRoot(make_shared<Node>()),
    m_pOrthoRoot(make_shared<Node>()),
    m_pSkyboxRoot(make_shared<Node>()),
    //m_pInterpreter(engine->interpreter()),
    //m_pScript(make_shared<Interpreter::Context>(engine->interpreter())),
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

    //auto l = make_shared<Light>();
    //l->dist(5.0f);
    //l->position(glm::vec3(0.0f, 2.0f, 0.0f));
    //m_pRoot->add(l);

    //auto particle = m_pQor->make<Particle>("particle.png");
    //m_pRoot->add(particle);
    
    auto mus = m_pQor->make<Sound>("cave.ogg");
    m_pRoot->add(mus);
    mus->ambient(true);

    m_pOrthoCamera = make_shared<Camera>(m_pQor->resources(), m_pQor->window());
    m_pOrthoCamera->ortho();
    m_pOrthoRoot->add(m_pOrthoCamera);
    
    m_pSkyboxCamera = make_shared<Camera>(m_pQor->resources(), m_pQor->window());
    m_pSkyboxCamera->perspective();
    m_pSkyboxRoot->add(m_pQor->make<Mesh>("skybox1.obj"));
    m_pSkyboxRoot->add(m_pSkyboxCamera);
    m_pSkyboxCamera->track(m_pCamera);
    m_pSkyboxCamera->mode(Tracker::ORIENT);
    
    auto tex = m_pQor->resources()->cache_cast<Texture>("crosshair2.png");
    auto crosshair = make_shared<Mesh>(
        make_shared<MeshGeometry>(
            Prefab::quad(
                -vec2((float)tex->center().x, (float)tex->center().y) / 2.0f,
                vec2((float)tex->center().x, (float)tex->center().y) / 2.0f
            )
        )
    );
    crosshair->add_modifier(make_shared<Wrap>(Prefab::quad_wrap(
        vec2(1.0f, -1.0f)
    )));
    crosshair->material(make_shared<MeshMaterial>(tex));
    crosshair->position(glm::vec3(win->center().x, win->center().y, 0.0f));
    m_pOrthoRoot->add(crosshair);
    
    //m_pPipeline = make_shared<Pipeline>(
    //    m_pQor->window(),
    //    m_pQor->resources(),
    //    m_pRoot,
    //    m_pCamera
    //);
    
    //m_pRoot->add(m_pQor->make<Mesh>("apartment_scene.obj"));
    auto scene = m_pQor->make<Scene>("thehall.json");
    m_pRoot->add(scene->root());
     
    // TODO: ensure filename contains only valid filename chars
    //m_pScript->execute_file("mods/"+ m_Filename +"/__init__.py");
    //m_pScript->execute_string("preload()");
    
    m_pPhysics->generate(m_pRoot.get(), (unsigned)Physics::GenerateFlag::RECURSIVE);
    m_pPhysics->world()->setGravity(btVector3(0.0, -40.0, 0.0));

    m_pConsole = make_shared<Console>(m_pQor->interpreter(), win, m_pInput, m_pQor->resources());
    m_pOrthoRoot->add(m_pConsole);
}

GameState :: ~GameState()
{
    //m_pPipeline->partitioner()->clear();
}

void GameState :: enter()
{
    m_pRoot->each([](Node* node){
        auto s = dynamic_cast<Sound*>(node);
        if(s) {
            s->play();
        }
    }, Node::Each::RECURSIVE);
    
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
        
    m_pSkyboxRoot->logic(t);
    m_pOrthoRoot->logic(t);
    m_pRoot->logic(t);

    //LOG(Matrix::to_string(*m_pSkyboxCamera->matrix(Space::WORLD)));
}

void GameState :: render() const
{
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
        m_pPlayer->camera(),
        nullptr,
        Pipeline::LIGHTS | Pipeline::NO_CLEAR
    );
    m_pPipeline->override_shader(PassType::NORMAL, (unsigned)PassType::NONE);
    m_pPipeline->winding(true);
    m_pPipeline->blend(true);
    m_pPipeline->render(
        m_pOrthoRoot.get(),
        m_pOrthoCamera.get(),
        nullptr,
        Pipeline::NO_CLEAR | Pipeline::NO_DEPTH
    );
    m_pPipeline->blend(false);
}


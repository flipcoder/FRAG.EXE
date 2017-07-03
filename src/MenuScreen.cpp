#include "MenuScreen.h"
#include "Qor/Input.h"
#include "Qor/Qor.h"
#include "Qor/TileMap.h"
#include "Qor/ScreenFader.h"
#include "Qor/Sound.h"
#include "Qor/Particle.h"
#include "kit/kit.h"
#include <glm/glm.hpp>
#include <cstdlib>
#include <chrono>
#include <thread>
#include "Qor/BasicPartitioner.h"
#include "Qor/Mesh.h"
#include "Qor/Material.h"
using namespace std;
using namespace glm;

MenuScreen :: MenuScreen(
    Qor* engine
    //std::string fn
):
    m_pQor(engine),
    m_pInput(engine->input()),
    m_pResources(engine->resources()),
    m_pController(m_pQor->session()->active_profile(0)->controller().get()),
    m_pPartitioner(engine->pipeline()->partitioner()),
    m_pRoot(make_shared<Node>()),
    m_pPipeline(engine->pipeline()),
    m_pCanvas(make_shared<Canvas>(
        engine->window()->size().x, engine->window()->size().y
    )),
    m_pMenuGUI(make_shared<MenuGUI>(
        engine->session()->active_profile(0)->controller().get(),
        &m_MenuContext,
        &m_MainMenu,
        m_pPipeline->partitioner(),
        m_pQor->window(),
        //m_pCanvas.get(),
        m_pResources,
        "RobotoCondensed-Regular.ttf",
        engine->window()->size().y / 30.0f,
        &m_Fade,
        7,
        1.5f,
        Text::LEFT,
        32.0f,
        MenuGUI::F_BOX
    ))
{
}

void MenuScreen :: preload()
{
    auto win = m_pQor->window();
    float sw = m_pQor->window()->size().x;
    float sh = m_pQor->window()->size().y;
    
    m_pCamera = make_shared<Camera>(m_pResources, m_pQor->window());
    m_pRoot->add(m_pCamera);
    m_pCamera->listen(true);
    m_pPipeline->blend(true);
    
    m_pMusic = m_pQor->make<Sound>("frag.ogg");
    m_pRoot->add(m_pMusic);
    
    m_Ambient.on_change.connect([this](const Color& c){
        int u = m_pPipeline->shader(1)->uniform("Ambient");
        if(u != -1)
            m_pPipeline->shader(1)->uniform(u, vec4(c.vec3(), c.a()));
    });
    
    on_tick.connect(std::move(screen_fader(
        [this](Freq::Time, float fade) {
            //m_Fade = fade;
            m_Ambient = Color(fade);
        },
        [this](Freq::Time){
            //return !!m_pDone;
            return false;
        },
        [this](Freq::Time){
            m_Ambient = Color::white();
            //if(m_pDone)
            //    (*m_pDone)();
            //else
            //    m_pQor->pop_state();
        }
    )));
    m_pRoot->add(m_pCanvas);

    auto logo = make_shared<Mesh>(
        make_shared<MeshGeometry>(
            Prefab::quad(
                -vec2(win->size().y, win->size().y)/2.0f,
                vec2(win->size().y, win->size().y)/2.0f
            )
        ));
    logo->add_modifier(make_shared<Wrap>(Prefab::quad_wrap(
        glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f)
    )));
    auto tex = m_pResources->cache_cast<ITexture>("logo.png");
    logo->material(make_shared<MeshMaterial>(tex));
    logo->move(vec3(win->center().x, win->center().y, -1.0f));
    m_pRoot->add(logo);

    auto mat = make_shared<Material>("fog_tiling.png", m_pResources);
    auto bg = make_shared<Mesh>(
        make_shared<MeshGeometry>(Prefab::quad(vec2(sw, sh), vec2(0.0f, 0.0f))),
        vector<shared_ptr<IMeshModifier>>{
            make_shared<Wrap>(Prefab::quad_wrap())
        },
        make_shared<MeshMaterial>(mat)
    );
    mat->diffuse(Color(1.0f, 0.25f));
    //auto bg2 = bg->instance();
    bg->position(vec3(0.0f,0.0f,-1.0f));
    //bg2->position(vec3(0.0f,0.0f,-1.0f));
    m_pRoot->add(bg);
    on_tick.connect([this, bg](Freq::Time t){
        m_WrapAccum.x += t.seconds() * 0.01f;
        m_WrapAccum.y += t.seconds() * 0.01f;
        
        m_WrapAccum += m_pInput->mouse_rel() * 0.0001f;
        m_WrapAccum.x = std::fmod(m_WrapAccum.x, 1.0f);
        m_WrapAccum.y = std::fmod(m_WrapAccum.y, 1.0f);
        
        bg->swap_modifier(0, make_shared<Wrap>(Prefab::quad_wrap(
             vec2(0.0f), vec2(1.0f),
             vec2(0.8f + 0.2f * m_Fade), //scale
             vec2(m_WrapAccum.x * 1.0f, m_WrapAccum.y * 1.0f) //offset
        )));
    });
}

MenuScreen :: ~MenuScreen()
{
    m_pPipeline->partitioner()->clear();
}

void MenuScreen :: enter()
{
    m_pCamera->ortho();
    m_pPipeline->winding(true);
    m_pPipeline->bg_color(Color::black());
    m_pMusic->play();
    
    m_pInput->relative_mouse(false);

    m_pPipeline->blend(true);
    
    m_MainMenu.options().emplace_back("PLAY ONLINE", [this]{
    });
    m_MainMenu.options().emplace_back("PRACTICE", [this]{
        m_pQor->change_state("pregame");
    });
    m_MainMenu.options().emplace_back("OPTIONS", [this]{
    });
    m_MainMenu.options().emplace_back("QUIT", [this]{
        m_pQor->pop_state();
    });
    
    m_MenuContext.clear(&m_MainMenu);
    m_pRoot->add(m_pMenuGUI);
}

void MenuScreen :: logic(Freq::Time t)
{
    Actuation::logic(t);
    m_pPipeline->logic(t);
    if(m_pInput->key(SDLK_F10))
        m_pQor->quit();
    m_pRoot->logic(t);
}

void MenuScreen :: render() const
{ 
    m_pPipeline->render(m_pRoot.get(), m_pCamera.get());
}


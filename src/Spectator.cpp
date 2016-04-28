#include "Spectator.h"
#include "HUD.h"
#include "Qor/Sound.h"
#include "Qor/Qor.h"
#include "Qor/Physics.h"
#include "Qor/Particle.h"
#include "Qor/Material.h"
#include "Game.h"
#include "Qor/BasicPartitioner.h"
using namespace std;
using namespace glm;

Spectator :: Spectator(
    Game* state,
    Node* root,
    shared_ptr<Profile> profile,
    Cache<Resource, string>* cache,
    Physics* physics,
    Window* window,
    Qor* engine,
    GameSpec* spec,
    NetSpec* net,
    glm::vec3 pos,
    std::function<bool()> lock_if
):
    m_pState(state),
    m_pRoot(root),
    m_pSpectator(make_shared<Node>()),
    m_pOrthoRoot(make_shared<Node>()),
    m_pProfile(profile),
    m_pController(profile->controller()),
    m_pCache(cache),
    m_pPhysics(physics),
    m_pWindow(window),
    m_pQor(engine),
    m_pSpec(spec),
    m_pNet(net),
    m_LockIf(lock_if)
{
    auto _this = this;
    
    m_pSpectator->position(pos);
    m_pRoot->add(m_pSpectator);
    m_pCamera = make_shared<Camera>(cache, window);
    m_pInterface = kit::init_shared<PlayerInterface3D>(
        m_pController,
        m_pCamera,
        m_pSpectator,
        m_pProfile->config(),
        [_this]{
            return (_this->m_LockIf && _this->m_LockIf());
        }
    );
    m_pInterface->speed(12.0f);
    m_pInterface->fly();
    m_pCamera->perspective();
    m_pSpectator->add(m_pCamera);
    m_pCamera->listen();
}

Spectator :: ~Spectator()
{
    //m_pInterface->unplug();
    m_pSpectator = nullptr;
}

void Spectator :: logic(Freq::Time t)
{
    m_pOrthoRoot->logic(t);
    
    if(m_LockIf && m_LockIf())
        return;
    
    if(
        m_pController->button("fire").pressed_now() ||
        m_pController->button("use").pressed_now() ||
        Headless::enabled() // TODO: for testing only
    ){
        if(m_pNet->local())
            m_pSpec->play();
        else
            m_pNet->spawn();
        return;
    }
}


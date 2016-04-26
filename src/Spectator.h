#ifndef SPECTATOR_H
#define SPECTATOR_H

#include "Qor/PlayerInterface3D.h"
#include "Qor/Camera.h"
#include "Qor/ViewModel.h"
#include "Qor/Mesh.h"
#include "Qor/Input.h"
#include "Qor/Window.h"
#include "Weapon.h"
#include "GameSpec.h"

class Qor;
class HUD;
class Game;

class Spectator:
    public IRealtime
{
    public:
        Spectator(
            Game* state,
            Node* root,
            std::shared_ptr<Profile> profile,
            Cache<Resource, std::string>* cache,
            Physics* physics,
            Window* window,
            Qor* engine,
            GameSpec* gamespec,
            NetSpec* net,
            glm::vec3 pos,
            std::function<bool()> lock_if = std::function<bool()>()
        );
        ~Spectator();
        
        virtual void logic(Freq::Time t) override;

        const std::shared_ptr<Camera>& camera() { return m_pCamera; }
        const std::shared_ptr<Node>& ortho_root() { return m_pOrthoRoot; }
        const std::shared_ptr<Camera>& ortho_camera() { return m_pOrthoCamera; }
        const std::shared_ptr<Controller>& controller() { return m_pController; }
        const std::shared_ptr<Profile>& profile() { return m_pProfile; }

        std::shared_ptr<Node> node() { return m_pSpectator; };
        
    private:
        
        Game* m_pState;
        Node* m_pRoot;
        std::shared_ptr<Node> m_pSpectator;
        std::shared_ptr<Node> m_pOrthoRoot;
        std::shared_ptr<Camera> m_pOrthoCamera;
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<Profile> m_pProfile;
        std::shared_ptr<Controller> m_pController;
        std::shared_ptr<PlayerInterface3D> m_pInterface;
        Cache<Resource, std::string>* m_pCache;
        Physics* m_pPhysics;
        Window* m_pWindow;
        Qor* m_pQor;
        std::function<bool()> m_LockIf;
        
        GameSpec* m_pSpec;
        NetSpec* m_pNet;

};

#endif


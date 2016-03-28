#ifndef _PLAYER_H_W5TRRMBU
#define _PLAYER_H_W5TRRMBU

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

class Player:
    public IRealtime
{
    public:
        Player(
            Node* root,
            std::shared_ptr<Controller> controller,
            Cache<Resource, std::string>* cache,
            Physics* physics,
            Window* window,
            Qor* engine,
            GameSpec* gamespec,
            std::function<bool()> lock_if = std::function<bool()>()
        );
        void logic(Freq::Time t);

        std::shared_ptr<Camera> camera() { return m_pCamera; }
        std::shared_ptr<Node> ortho_root() { return m_pOrthoRoot; }
        std::shared_ptr<Camera> ortho_camera() { return m_pOrthoCamera; }

        bool can_jump() const;

        std::shared_ptr<Mesh> mesh() { return m_pPlayerMesh; }
        
        //void stand(glm::vec3 pos);
        
    private:
        
        void scope(bool b);
        void decal(glm::vec3 contact, glm::vec3 normal, glm::vec3 up, float offset);
        void refresh_weapon();
        
        Node* m_pRoot;
        std::shared_ptr<Node> m_pOrthoRoot;
        std::shared_ptr<Camera> m_pOrthoCamera;
        std::shared_ptr<Mesh> m_pPlayerMesh;
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<ViewModel> m_pViewModel;
        std::shared_ptr<Controller> m_pController;
        std::shared_ptr<PlayerInterface3D> m_pInterface;
        std::shared_ptr<Mesh> m_pCrosshair;
        std::shared_ptr<HUD> m_pHUD;
        Cache<Resource, std::string>* m_pCache;
        Physics* m_pPhysics;
        Window* m_pWindow;
        Qor* m_pQor;
        std::function<bool()> m_LockIf;

        GameSpec* m_pGameSpec;
        WeaponStash m_WeaponStash;

        std::shared_ptr<ITexture> m_pDecal;
        std::shared_ptr<ITexture> m_pSpark;
        std::deque<std::shared_ptr<Mesh>> m_Decals;
        static const unsigned MAX_DECALS;

        bool m_bScope = false;
        float m_fFOV;
};

#endif


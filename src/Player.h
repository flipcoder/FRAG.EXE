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
class GameState;

class Player:
    public IRealtime
{
    public:
        Player(
            GameState* state,
            Node* root,
            std::shared_ptr<Controller> controller,
            Cache<Resource, std::string>* cache,
            Physics* physics,
            Window* window,
            Qor* engine,
            GameSpec* gamespec,
            std::function<bool()> lock_if = std::function<bool()>()
        );
        
        ~Player();
        
        void logic(Freq::Time t);

        std::shared_ptr<Camera> camera() { return m_pCamera; }
        std::shared_ptr<Node> ortho_root() { return m_pOrthoRoot; }
        std::shared_ptr<Camera> ortho_camera() { return m_pOrthoCamera; }

        bool can_jump() const;

        std::shared_ptr<Mesh> mesh() { return m_pPlayerMesh; }
        
        void die();
        void hurt(int dmg);
        bool dead();
        bool alive();
        void reset();
        void crouch(bool b);
        
    private:
        
        void update_hud();
        void scope(bool b);
        void decal(glm::vec3 contact, glm::vec3 normal, glm::vec3 up, float offset);
        void refresh_weapon();
        
        GameState* m_pState;
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
        bool m_bCrouched = false;
        bool m_bEnter = false;
        float m_fFOV;

        Color m_FlashColor;
        Freq::Alarm m_FlashAlarm;

        Box m_StandBox;
        Box m_CrouchBox;

};

#endif


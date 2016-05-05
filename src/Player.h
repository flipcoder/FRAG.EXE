#ifndef _PLAYER_H_W5TRRMBU
#define _PLAYER_H_W5TRRMBU

#include "Qor/PlayerInterface3D.h"
#include "Qor/Camera.h"
#include "Qor/ViewModel.h"
#include "Qor/Mesh.h"
#include "Qor/Input.h"
#include "Qor/Window.h"
#include "Qor/Profile.h"
#include "Weapon.h"
#include "GameSpec.h"

class Qor;
class HUD;
class Game;
class NetSpec;

class Player:
    public IRealtime,
    public std::enable_shared_from_this<Player>
{
    public:
        Player(
            Game* state,
            Node* root,
            std::shared_ptr<Profile> profile,
            //std::shared_ptr<Controller> controller,
            Cache<Resource, std::string>* cache,
            Physics* physics,
            Window* window,
            Qor* engine,
            GameSpec* gamespec,
            NetSpec* net,
            glm::vec3 pos, // position if no spawn point
            std::function<bool()> lock_if = std::function<bool()>()
        );
        
        ~Player();
        
        virtual void logic(Freq::Time t) override;

        enum PlayerEvent
        {
            PE_FIRE,
            PE_NEXT,
            PE_PREV,
            PE_RELOAD,
            PE_HURT,
            PE_SLOT
        };

        void do_event(unsigned char ev);
        void fire_weapon();
        void reload();
        void next_weapon();
        void prev_weapon();
        bool slot(unsigned);
        
        const std::shared_ptr<Camera>& camera() { return m_pCamera; }
        const std::shared_ptr<Node>& ortho_root() { return m_pOrthoRoot; }
        const std::shared_ptr<Camera>& ortho_camera() { return m_pOrthoCamera; }
        const std::shared_ptr<Controller>& controller() { return m_pController; }
        const std::shared_ptr<Profile>& profile() { return m_pProfile; }

        bool can_jump() const;

        std::shared_ptr<Mesh> shape() { return m_pPlayerShape; }
        
        void die();
        void hurt(int dmg);
        void heal(int hp) { hurt(-hp); }
        bool dead();
        bool alive();
        void reset();
        bool crouch(bool b);
        void jump();
        void give(const std::shared_ptr<Meta>& item);
        void update_hud();
        void add_frags(Player* target, int f = 1);

        bool local() const { return !!m_pController; }

        std::string name() const { return m_pProfile->name(); }
        
        boost::signals2::signal<void()> on_death;
        
        std::string death_msg() const { return m_DeathMsg; }
        
        void unpack_transform(glm::mat4 m);
        glm::mat4 pack_transform();

        boost::signals2::signal<void(unsigned char)> on_event;
        boost::signals2::signal<void(unsigned)> on_slot;
        
    private:
        
        void scope(bool b);
        void decal(Node* n, glm::vec3 contact, glm::vec3 normal, glm::vec3 up, float offset);
        void refresh_weapon();
        bool weapon_priority_cmp(std::string s1, std::string s2);

        Game* m_pState;
        Node* m_pRoot;
        std::shared_ptr<Node> m_pOrthoRoot;
        std::shared_ptr<Camera> m_pOrthoCamera;
        std::shared_ptr<Mesh> m_pPlayerShape;
        std::shared_ptr<Mesh> m_pPlayerModel;
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<ViewModel> m_pViewModel;
        std::shared_ptr<Profile> m_pProfile;
        std::shared_ptr<Controller> m_pController;
        std::shared_ptr<PlayerInterface3D> m_pInterface;
        std::shared_ptr<Mesh> m_pCrosshair;
        std::shared_ptr<HUD> m_pHUD;
        Cache<Resource, std::string>* m_pCache;
        Physics* m_pPhysics;
        Window* m_pWindow;
        Qor* m_pQor;
        std::function<bool()> m_LockIf;

        GameSpec* m_pSpec;
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

        boost::signals2::scoped_connection m_HPChangeCon;

        NetSpec* m_pNet;

        glm::mat4 m_NetTransform;
        
        std::string m_DeathMsg;
};

#endif


#ifndef _GAMESPEC_H
#define _GAMESPEC_H

#include <memory>
#include <string>
#include "Qor/kit/meta/meta.h"
#include "Qor/kit/cache/cache.h"
#include "Qor/Resource.h"
#include "Qor/BasicPartitioner.h"
#include "Weapon.h"
class Node;
class Player;
class Qor;
class Spectator;
class GameState;

class GameSpec:
    public IRealtime
{
    public:
        GameSpec(std::string fn, Cache<Resource, std::string>* cache,
            Node* root, BasicPartitioner* part,
            std::shared_ptr<Controller> ctrl,
            Qor* engine, GameState* state
        );
        WeaponSpec* weapons() { return &m_WeaponSpec; }

        void set_physics(Physics* p) { m_pPhysics = p; }

        void setup();
        void register_player(std::shared_ptr<Player> p);
        void deregister_player(Player* p);
        std::shared_ptr<Meta> config() { return m_pConfig; };
        
        void play(std::shared_ptr<Controller> ctrl = nullptr);
        bool respawn(Player* p);
        void despawn(Player* p);
        void spectate(std::shared_ptr<Controller> ctrl = nullptr);

        void spawn_local_player();
        void spawn_local_spectator();
        void set_lock(std::function<bool()> func) { m_LockIf = func; }

        Player* local_player() { return m_pPlayer; }
        Spectator* local_spectator() { return m_pSpectator.get(); }

        const std::shared_ptr<Camera>& camera() const { return m_pCamera; }
        const std::shared_ptr<Camera>& ortho_camera() const { return m_pOrthoCamera; }
        std::shared_ptr<Node> ortho_root() const;

        virtual void logic(Freq::Time t) override;
        
    private:
        
        void register_pickup_with_player(std::shared_ptr<Mesh> item, Player* player);
        
        std::shared_ptr<Meta> m_pConfig;
        Cache<Resource, std::string>* m_pCache;
        WeaponSpec m_WeaponSpec;
        Node* m_pRoot;
        BasicPartitioner* m_pPartitioner;
        Qor* m_pQor;
        Physics* m_pPhysics = nullptr;
        GameState* m_pState;

        std::vector<std::shared_ptr<Player>> m_Players;
        Player* m_pPlayer = nullptr; // local
        std::shared_ptr<Spectator> m_pSpectator = nullptr; // local
        std::vector<std::shared_ptr<Mesh>> m_WeaponPickups;
        std::vector<std::shared_ptr<Mesh>> m_ItemPickups;
        std::shared_ptr<Controller> m_pController;
        
        // local player cameras
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<Camera> m_pOrthoCamera;

        std::function<bool()> m_LockIf;
};

#endif


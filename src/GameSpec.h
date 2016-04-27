#ifndef _GAMESPEC_H
#define _GAMESPEC_H

#include <memory>
#include <string>
#include "Qor/kit/meta/meta.h"
#include "Qor/kit/cache/cache.h"
#include "Qor/Resource.h"
#include "Qor/BasicPartitioner.h"
#include "Weapon.h"
#include "NetSpec.h"
class Node;
class Player;
class Qor;
class Spectator;
class Game;
class Profile;

class GameSpec:
    public IRealtime
{
    public:
        GameSpec(std::string fn, Cache<Resource, std::string>* cache,
            Node* root, BasicPartitioner* part,
            std::shared_ptr<Profile> prof,
            Qor* engine, Game* state,
            NetSpec* net
        );
        WeaponSpec* weapons() { return &m_WeaponSpec; }

        void set_physics(Physics* p) { m_pPhysics = p; }

        void setup();
        void register_player(std::shared_ptr<Player> p);
        void deregister_player(Player* p);
        std::shared_ptr<Meta> config() { return m_pConfig; };
        
        Player* play(std::shared_ptr<Profile> prof = nullptr);
        bool respawn(Player* p);
        void despawn(Player* p);
        void spectate(std::shared_ptr<Profile> prof = nullptr);

        void spawn_local_player();
        void spawn_local_spectator();
        void set_lock(std::function<bool()> func) { m_LockIf = func; }

        Player* local_player() { return m_pPlayer; }
        Spectator* local_spectator() { return m_pSpectator.get(); }

        const std::shared_ptr<Camera>& camera() const { return m_pCamera; }
        const std::shared_ptr<Camera>& ortho_camera() const { return m_pOrthoCamera; }
        std::shared_ptr<Node> ortho_root() const;

        virtual void logic(Freq::Time t) override;

        boost::signals2::signal<void(Player*)> on_player_spawn;
        boost::signals2::signal<void(Spectator*)> on_spectator_spawn;

        //void net(NetSpec* net) { m_pNet = net; }
        NetSpec* net() { return m_pNet; }
        
        // spawn something according to packet data
        void spawn(RakNet::Packet* packet);
        
    private:
        
        void register_pickup_with_player(std::shared_ptr<Mesh> item, Player* player);
        
        std::shared_ptr<Meta> m_pConfig;
        Cache<Resource, std::string>* m_pCache;
        WeaponSpec m_WeaponSpec;
        Node* m_pRoot;
        BasicPartitioner* m_pPartitioner;
        Qor* m_pQor;
        Physics* m_pPhysics = nullptr;
        Game* m_pState;

        std::vector<std::shared_ptr<Player>> m_Players;
        Player* m_pPlayer = nullptr; // local
        std::shared_ptr<Spectator> m_pSpectator = nullptr; // local
        std::vector<std::shared_ptr<Mesh>> m_WeaponPickups;
        std::vector<std::shared_ptr<Mesh>> m_ItemPickups;
        std::shared_ptr<Profile> m_pProfile;
        std::shared_ptr<Controller> m_pController;
        
        // local player cameras
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<Camera> m_pOrthoCamera;

        NetSpec* m_pNet = nullptr;

        std::function<bool()> m_LockIf;
};

#endif


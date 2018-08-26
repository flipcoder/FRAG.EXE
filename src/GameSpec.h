#ifndef _GAMESPEC_H
#define _GAMESPEC_H

#include <memory>
#include <string>
#include "kit/meta/meta.h"
#include "kit/cache/cache.h"
#include "Qor/Resource.h"
#include "Qor/BasicPartitioner.h"
#include "Weapon.h"
#include "NetSpec.h"
class ResourceCache;
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
        GameSpec(std::string fn, ResourceCache* cache,
            Node* root, BasicPartitioner* part,
            std::shared_ptr<Profile> prof,
            Qor* engine, Game* state,
            NetSpec* net
        );
        WeaponSpec* weapons() { return &m_WeaponSpec; }

        void set_physics(Physics* p) { m_pPhysics = p; }

        void enter();
        void setup();
        void register_player(std::shared_ptr<Player> p);
        void register_projectile(std::shared_ptr<Node> p, std::function<void(Node*,Node*)>);
        void deregister_player(Player* p);
        std::shared_ptr<Meta> config() { return m_pConfig; };
        
        Player* play(std::shared_ptr<Profile> prof = nullptr);
        void spawn(Player* player);
        void respawn(Player* player);
        bool teleport_to_spawn(Player* p);
        //void despawn(Player* p);
        void spectate(std::shared_ptr<Profile> prof = nullptr);

        void spawn_local_player();
        void spawn_local_spectator();
        void set_lock(std::function<bool()> func) { m_LockIf = func; }
        void map(std::string map) { m_Map = map; }

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
        void client_spawn(RakNet::Packet* packet);
        void server_notify_spawn(RakNet::Packet* p, bool prior);
        //void client_despawn(RakNet::Packet* packet);
        //void server_despawn(Player* p);
        void send_update(Player* p);
        void recv_update(RakNet::Packet* p);
        void client_done_loading();
        void recv_player_event(RakNet::Packet* p);
        void send_player_event(Player* p, unsigned char ev);
        void send_player_event_slot(Player* p, unsigned slot);
        void send_player_event_hurt(Player* p, int dmg);
        void send_spawn(Player* p = nullptr);
        void send_player_event_frag(Player* p, Player* target);
        void send_player_event_give(Player* p, std::string what);
        void weapon_pickup(Player* p, Node* item);
        
        void splash(Node* m, std::shared_ptr<Meta> hitinfo);
        
    private:
        
        void register_pickup_with_player(std::shared_ptr<Mesh> item, Player* player);
        
        std::shared_ptr<Meta> m_pConfig;
        ResourceCache* m_pCache;
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
        std::string m_Map;
        
        // local player cameras
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<Camera> m_pOrthoCamera;

        NetSpec* m_pNet = nullptr;

        std::function<bool()> m_LockIf;
        
        boost::signals2::scoped_connection m_SpawnCon;
        boost::signals2::scoped_connection m_DespawnCon;
        boost::signals2::scoped_connection m_UpdateCon;
        boost::signals2::scoped_connection m_DoneLoadingCon;
        boost::signals2::scoped_connection m_PlayerEventCon;
};

#endif


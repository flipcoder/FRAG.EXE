#ifndef NETSPEC_H
#define NETSPEC_H

#include "kit/kit.h"
#include "Qor/Session.h"
#include "Qor/Profile.h"
#include "Qor/Net.h"
#include "Qor/Node.h"
#include "raknet/MessageIdentifiers.h"

class NetSpec:
    public Session::IModule
{
    public:

        enum PacketType
        {
            ID_INFO = ID_USER_PACKET_ENUM,
            ID_MSG,
            ID_SPAWN,
            ID_DESPAWN,
            ID_UPDATE,
            ID_PLAYER_EVENT,
            ID_STATE_EVENT,
            ID_GIVE,
            ID_DONE_LOADING
        };

        enum ObjectType
        {
            OBJ_EMPTY,
            OBJ_PLAYER
        };
        
        NetSpec() { assert(false); }
        NetSpec(Qor* engine, bool server, int connections = 8);
        virtual ~NetSpec();
        virtual void logic(Freq::Time t) override;

        Net* net() { return m_pNet.get(); }

        bool remote() { return m_pNet->remote(); }
        bool local() { return m_pNet->local(); }
        bool server() { return m_pNet->server(); }

        void message(
            std::string msg,
            RakNet::RakNetGUID guid = RakNet::UNASSIGNED_RAKNET_GUID
        );
        void info(
            std::string info, uint32_t object_id = 0,
            std::string name = "",
            RakNet::RakNetGUID guid = RakNet::UNASSIGNED_RAKNET_GUID
        );
        void data(RakNet::Packet* packet);
        void disconnect(RakNet::Packet* packet);
        void spawn(RakNet::RakNetGUID guid = RakNet::UNASSIGNED_RAKNET_GUID);
        
        std::string client_name(RakNet::RakNetGUID guid) const;

        void reserve(unsigned id);
        void add_object(unsigned id, std::shared_ptr<Node> node);
        void remove_object(unsigned id);
        bool has_object(unsigned id) { return m_Nodes.has(id); }

        RakNet::RakPeerInterface* socket() { return m_pNet->socket(); }

        boost::signals2::signal<void(RakNet::Packet*)> on_info;
        boost::signals2::signal<void(RakNet::Packet*)> on_spawn;
        boost::signals2::signal<void(RakNet::Packet*)> on_despawn;
        boost::signals2::signal<void(RakNet::Packet*)> on_update;
        boost::signals2::signal<void(RakNet::Packet*)> on_done_loading;
        boost::signals2::signal<void(RakNet::Packet*)> on_player_event;
        
        unsigned get_object_id_for(RakNet::RakNetGUID id) const {
            return m_Profiles.at(id)->temp()->at<int>("id");
        }
        
        std::shared_ptr<Profile> profile(RakNet::RakNetGUID guid) {
            return m_Profiles[guid];
        }

        std::shared_ptr<Node> object(unsigned id) {
            return m_Nodes.at(id);
        }

        std::map<RakNet::RakNetGUID, std::shared_ptr<Profile>>& profiles() {
            return m_Profiles;
        }
        
        kit::shared_index<Node>& nodes() {
            return m_Nodes;
        }
        
    private:

        void server_recv_disconnect(RakNet::Packet*);
        void client_recv_disconnect(RakNet::Packet*);
        
        kit::shared_index<Node> m_Nodes;
        std::map<RakNet::RakNetGUID, std::shared_ptr<Profile>> m_Profiles;
        
        std::shared_ptr<Net> m_pNet;
        boost::signals2::scoped_connection m_DataCon;
        boost::signals2::scoped_connection m_DisconnectCon;
        boost::signals2::scoped_connection m_TimeoutCon;
        boost::signals2::scoped_connection m_ConnectionLostCon;
        Session* m_pSession;
};

#endif


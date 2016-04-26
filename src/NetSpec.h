#ifndef NETSPEC_H
#define NETSPEC_H

#include "Qor/kit/kit.h"
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
            ID_CHANGE,
            ID_EVENT
        };
        
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
        void info(std::string info, uint32_t object_id = 0, std::string name = "");
        void data(RakNet::Packet* packet);
        void disconnect(RakNet::Packet* packet);
        std::string client_name(RakNet::RakNetGUID guid) const;

        void reserve(unsigned id);

        RakNet::RakPeerInterface* socket() { return m_pNet->socket(); }

        boost::signals2::signal<void(RakNet::Packet*)> on_info;
        boost::signals2::signal<void(RakNet::Packet*)> on_spawn;
        boost::signals2::signal<void(RakNet::Packet*)> on_despawn;
        boost::signals2::signal<void(RakNet::Packet*)> on_change;
        
        unsigned get_object_id_for(RakNet::RakNetGUID id) const {
            return m_Profiles.at(id)->temp()->at<int>("id");
        }
        
        std::shared_ptr<Profile> profile(RakNet::RakNetGUID guid) {
            return m_Profiles[guid];
        }
        
    private:
        
        kit::shared_index<Node> m_Nodes;
        std::map<RakNet::RakNetGUID, std::shared_ptr<Profile>> m_Profiles;
        
        std::shared_ptr<Net> m_pNet;
        boost::signals2::scoped_connection m_DataCon;
        boost::signals2::scoped_connection m_DisconnectCon;
        Session* m_pSession;
};

#endif


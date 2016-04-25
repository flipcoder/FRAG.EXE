#ifndef NETSPEC_H
#define NETSPEC_H

#include "Qor/Session.h"
#include "Qor/Net.h"

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
        void data(RakNet::Packet* packet);

        RakNet::RakPeerInterface* socket() { return m_pNet->socket(); }
        
    private:
        
        std::shared_ptr<Net> m_pNet;
        boost::signals2::scoped_connection m_DataCon;
};

#endif


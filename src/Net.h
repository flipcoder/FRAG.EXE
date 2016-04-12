#ifndef _NET_H_RKPLEW0
#define _NET_H_RKPLEW0
#include <raknet/RakPeer.h>
#include "Qor/IRealtime.h"
#include "Qor/Session.h"

class Qor;

class Net:
    public Session::IModule
{
    public:
        Net(Qor* engine, bool server);
        virtual ~Net();
        
        void destroy();

        virtual void logic(Freq::Time t) override;
        
    public:
        
        RakNet::RakPeerInterface* m_pSocket = nullptr;
};

#endif


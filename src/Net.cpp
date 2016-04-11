#include <raknet/RakPeer.h>
#include "Net.h"

#define MAX_CONNECTIONS 8
using namespace RakNet;

Net :: Net()
{
    try{
        RakPeerInterface* peer = RakNet::RakPeerInterface::GetInstance();
        SocketDescriptor sd(11523, 0);
        m_pPeer->Startup(MAX_CONNECTIONS, &sd, 1);
    }catch(...){
        destroy();
        throw;
    }
}

void Net :: destroy()
{
    if(m_pPeer)
        RakPeerInterface::DestroyInstance(m_pPeer);
}

Net :: ~Net()
{
    destroy();
}


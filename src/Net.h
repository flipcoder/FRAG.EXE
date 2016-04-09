#ifndef _NET_H_RKPLEW0
#define _NET_H_RKPLEW0

class Net
{
    public:
        Net();
        ~Net();
        
        void destroy();

    public:
        
        RakNet::RakPeerInterface* m_pPeer;
};

#endif


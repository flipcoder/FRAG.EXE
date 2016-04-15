#include "Net.h"
#include "Qor/Qor.h"
#include "raknet/MessageIdentifiers.h"

#define MAX_CONNECTIONS 8
using namespace RakNet;
using namespace std;

Net :: Net(Qor* engine, bool server)
{
    try{
        if(not server){
            string ip = engine->args().value("ip").c_str();
            if(not ip.empty()){
                LOGf("Connecting to %s...", ip);
                m_pSocket = RakNet::RakPeerInterface::GetInstance();
                SocketDescriptor sd(0, 0);
                sd.socketFamily=AF_INET;
                m_pSocket->Startup(MAX_CONNECTIONS, &sd, 1);
                m_pSocket->Connect(ip.c_str(), 11523, 0, 0);
                m_pSocket->SetOccasionalPing(true);
            }
        } else {
            LOG("Starting server...");
            m_pSocket = RakNet::RakPeerInterface::GetInstance();
            SocketDescriptor sd(11523, 0);
            sd.socketFamily=AF_INET;
            m_pSocket->Startup(MAX_CONNECTIONS, &sd, 1);
            m_pSocket->SetMaximumIncomingConnections(MAX_CONNECTIONS);
            m_pSocket->SetOccasionalPing(true);
            m_pSocket->SetUnreliableTimeout(1000);
        }
    }catch(...){
        destroy();
        throw;
    }
}

void Net :: destroy()
{
    if(m_pSocket){
        m_pSocket->Shutdown(300);
        RakPeerInterface::DestroyInstance(m_pSocket);
    }
}

Net :: ~Net()
{
    destroy();
}

void Net :: logic(Freq::Time t)
{
    if(not m_pSocket)
        return;
    
    RakNet::Packet *packet;
    while(true)
    {
        packet = m_pSocket->Receive();
        if(not packet)
            break;
        
        try{
            switch(packet->data[0])
            {
                case ID_CONNECTION_REQUEST_ACCEPTED:
                    LOG("Connected.");
                    break;
                case ID_NO_FREE_INCOMING_CONNECTIONS:
                    LOG("No free incoming connections.");
                    break;
                case ID_DISCONNECTION_NOTIFICATION:
                    LOG("Disconnected.");
                    break;
                case ID_CONNECTION_LOST:
                    LOG("Connection lost.");
                    break;
                case ID_CONNECTION_ATTEMPT_FAILED:
                    LOG("Connection attempt failed.");
                    break;
                case ID_SND_RECEIPT_ACKED:
                    LOG("Send receipt acked.");
                    break;
                case ID_SND_RECEIPT_LOSS:
                    LOG("Send receipt loss.");
                    break;

                // server only?
                case ID_NEW_INCOMING_CONNECTION:
                    LOG("New incoming connection.");
                    break;
                case ID_INCOMPATIBLE_PROTOCOL_VERSION:
                    LOG("Incompatible protocol version.");
                    break;
                case ID_CONNECTED_PING:
                    LOG("Ping");
                    break;
                case ID_UNCONNECTED_PING:
                    LOG("Ping (unconnected)");
                    break;
                default:
                    LOG("Unknown packet data.");
                    break;
            }
            m_pSocket->DeallocatePacket(packet);
        }catch(...){
            m_pSocket->DeallocatePacket(packet);
            throw;
        }
    }
}


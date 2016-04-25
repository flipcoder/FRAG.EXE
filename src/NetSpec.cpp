#include "NetSpec.h"
#include <functional>
using namespace std;
using namespace RakNet;

NetSpec :: NetSpec(Qor* engine, bool server, int connections):
    m_pNet(make_shared<Net>(engine,server,connections))
{
    m_DataCon = m_pNet->on_data.connect(std::bind(&NetSpec::data, this, placeholders::_1));
}

NetSpec :: ~NetSpec() {}

void NetSpec :: logic(Freq::Time t)
{
    m_pNet->logic(t);
}

//void NetSpec :: send(Bistream* bs)
//{
    
//}

void NetSpec :: message(std::string msg, RakNetGUID guid)
{
    if(not local())
    {
        BitStream bs;
        bs.Write((unsigned char)ID_MSG);
        bs.Write(RakString(msg.c_str()));
        m_pNet->socket()->Send(
            &bs,
            MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, guid, true
        );
    }
}

void NetSpec :: data(Packet* packet)
{
    auto guid = RakNetGUID::ToUint32(packet->guid);
    
    BitStream bs(packet->data, packet->length, false);
    RakString rs;
    unsigned char id;
    bs.Read(id);

    if(id == ID_MSG)
    {
        bs.Read(rs);
        std::string s = rs.C_String();
        string msg = string("Client ") + to_string(guid) + ": "+ s;
        LOG(msg);
        if(server())
            message(msg, packet->guid);
    }
}


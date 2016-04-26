#include "Qor/Qor.h"
#include "NetSpec.h"
#include <functional>
using namespace std;
using namespace RakNet;

NetSpec :: NetSpec(Qor* engine, bool server, int connections):
    m_pNet(make_shared<Net>(engine,server,connections)),
    m_pSession(engine->session())
{
    m_DataCon = m_pNet->on_data.connect(std::bind(&NetSpec::data, this, placeholders::_1));
    m_DisconnectCon = m_pNet->on_disconnect.connect(std::bind(&NetSpec::disconnect, this, placeholders::_1));
}

NetSpec :: ~NetSpec() {}

void NetSpec :: logic(Freq::Time t)
{
    m_pNet->logic(t);
    nodes.optimize();
}

//void NetSpec :: send(Bistream* bs)
//{
    
//}

void NetSpec :: disconnect(Packet* packet)
{
    //auto guid = RakNetGUID::ToUint32(packet->guid);
    BitStream bs(packet->data, packet->length, false);
    try{
        LOGf("%s disconnected.", profiles.at(packet->guid)->name());
    }catch(const std::out_of_range&){}
}

void NetSpec :: info(std::string info, uint32_t object_id)
{
    // for server:
    //   string - map name
    //   uint32_t - client id
    // client:
    //   string - client name
    
    BitStream bs;
    bs.Write((unsigned char)ID_INFO);
    bs.Write(RakString(info.c_str()));
    if(server())
        bs.Write(object_id);
    m_pNet->socket()->Send(
        &bs,
        MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_RAKNET_GUID, true
    );
}

void NetSpec :: message(std::string msg, RakNetGUID guid)
{
    // string - message
    
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

    if(id == ID_CHANGE)
    {
        // change the state of an object
        return;
    }
    else if(id == ID_DESPAWN)
    {
        // despawn an object
        if(remote())
        {
        }
        return;
    }
    else if(id == ID_SPAWN)
    {
        if(server())
        {
            // client is requesting spawn of self or object
            on_spawn(packet);
        }
        else if(remote())
        {
            // server is spawning something 
            on_spawn(packet);
        }
        return;
    }
    else if(id == ID_MSG)
    {
        bs.Read(rs);
        std::string s = rs.C_String();
        string name = client_name(packet->guid);
        if(name.empty())
            name = string("Client ") + to_string(guid);
        string msg = name + ": "+ s;
        LOG(msg);
        if(server())
            message(msg, packet->guid);
        return;
    }
    else if(id == ID_INFO)
    {
        if(server())
        {
            LOG("recv info");
            bs.Read(rs);
            std::string name = rs.C_String();
            bool name_used = false;
            if(client_name(packet->guid).empty()){
                for(auto&& p: m_Profiles)
                    if(name == p.second->name()){
                        name_used = true;
                        break;
                    }
                if(not name_used){
                    auto prof = m_Profiles[packet->guid] = m_pSession->dummy_profile(name);
                    prof->temp()->set<int>("id", m_Nodes.reserve_next());
                }
            }else{
                // change name
                m_Profiles[packet->guid]->name(name);
            }
        }
        on_info(packet);
        return;
    }
}

string NetSpec :: client_name(RakNetGUID guid) const
{
    try{
        return profiles.at(guid)->name();
    }catch(...){
        return string();
    }
}

void NetSpec :: reserve(unsigned id)
{
    m_Nodes.reserve(id);
}


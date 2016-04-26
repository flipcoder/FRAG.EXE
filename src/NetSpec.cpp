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
    m_Nodes.optimize();
}

//void NetSpec :: send(Bistream* bs)
//{
    
//}

void NetSpec :: disconnect(Packet* packet)
{
    //auto guid = RakNetGUID::ToUint32(packet->guid);
    //BitStream bs(packet->data, packet->length, false);
    try{
        LOGf("%s disconnected.", m_Profiles.at(packet->guid)->name());
    }catch(const std::out_of_range&){}
}

void NetSpec :: info(std::string info, uint32_t object_id, std::string name)
{
    // for server:
    //   string - map name
    //   uint32_t - client id
    //   string - actual client name (might be different)
    // client:
    //   string - client name
    
    BitStream bs;
    bs.Write((unsigned char)ID_INFO);
    bs.Write(RakString(info.c_str()));
    
    if(server())
    {
        bs.Write(object_id);
        bs.Write(RakString(name.c_str()));
    }
    
    m_pNet->socket()->Send(
        &bs,
        MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_RAKNET_GUID, true
    );
}

void NetSpec :: message(std::string msg, RakNetGUID guid)
{
    // string - message
    
    LOG(msg);
    
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
        on_change(packet);
        return;
    }
    else if(id == ID_DESPAWN)
    {
        on_despawn(packet);
        return;
    }
    else if(id == ID_SPAWN)
    {
        on_spawn(packet);
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
            
            // name not set?
            if(client_name(packet->guid).empty()){
                // name unused?
                while(true){
                    for(auto&& p: m_Profiles)
                        if(name == p.second->name()){
                            name_used = true;
                            break;
                        }
                    if(not name_used){
                        auto prof = m_Profiles[packet->guid] = m_pSession->dummy_profile(name);
                        prof->temp()->set<int>("id", m_Nodes.reserve_next());
                        break;
                    }
                    name += "_";
                }
            }else{
                // change name
                auto prof = m_Profiles[packet->guid];
                for(auto&& p: m_Profiles)
                    if(name == p.second->name()){
                        name_used = true;
                        break;
                    }

                if(not name_used){
                    string old_name = prof->name();
                    prof->name(name);
                    message(old_name + " is now known as " + name);
                }
            }
            message(name + " connected.");
        }
        on_info(packet);
        return;
    }
}

string NetSpec :: client_name(RakNetGUID guid) const
{
    try{
        return m_Profiles.at(guid)->name();
    }catch(...){
        return string();
    }
}

void NetSpec :: reserve(unsigned id)
{
    m_Nodes.reserve(id);
}


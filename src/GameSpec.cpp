#include "Qor/Mesh.h"
#include "Qor/Node.h"
#include "Player.h"
#include "Spectator.h"
#include "Qor/Qor.h"
#include "Game.h"
#include <boost/regex.hpp>
#include "Qor/Profile.h"
using namespace std;
using namespace glm;
using namespace RakNet;

GameSpec :: GameSpec(std::string fn, Cache<Resource, std::string>* cache,
    Node* root, BasicPartitioner* part,
    shared_ptr<Profile> prof,
    Qor* engine, Game* state,
    NetSpec* net
):
    m_pConfig(make_shared<Meta>(cache->transform(fn))),
    m_pCache(cache),
    m_WeaponSpec(m_pConfig->meta("weapons")),
    m_pRoot(root),
    m_pPartitioner(part),
    m_pProfile(prof),
    m_pController(prof->controller()),
    m_pQor(engine),
    m_pState(state),
    m_pNet(net)
{}

void GameSpec :: register_player(shared_ptr<Player> p)
{
    m_Players.push_back(p);

    if(not m_pNet->remote()){
        for(auto&& wpn: m_WeaponPickups)
            register_pickup_with_player(wpn, p.get());
        for(auto&& item: m_ItemPickups)
            register_pickup_with_player(item, p.get());
    }

    if(m_pNet->server())
    {
        p->on_death.connect(bind(&GameSpec::server_despawn, this, p.get()));
        p->shape()->on_move.connect(bind(&GameSpec::send_update, this, p.get()));
    }
    else if(m_pNet->remote() && p->local())
        p->shape()->on_move.connect(bind(&GameSpec::send_update, this, p.get()));
}

void GameSpec :: deregister_player(Player* p)
{
    kit::remove(m_Players, p->shared_from_this());
    if(p == m_pPlayer)
        m_pPlayer = nullptr;
}

void GameSpec :: register_pickup_with_player(std::shared_ptr<Mesh> item, Player* player)
{
    auto part = m_pPartitioner;
    m_pPartitioner->on_touch(item, player->shape(), [part, player](Node* a, Node* b){
        assert(a && b);
        if(not a->attached() && not a->detaching())
            return;
        // spawn item with spawn
        a->config()->set<string>("name", a->name());
        player->give(a->config());
        a->detach();
        //part->deregister_object(a->as_node());
    });
}

void GameSpec :: setup()
{
    // cache
    for(auto&& wpn: m_WeaponSpec)
        make_shared<Mesh>(m_pCache->transform(wpn.second.model()), m_pCache);
    
    for(auto&& wpn: m_WeaponSpec)
    {
        // find spawns on map
        auto spawns = m_pRoot->hook(wpn.second.name() + string(".*"), Node::Hook::REGEX);
        for(auto&& spawn: spawns)
        {
            auto m = make_shared<Mesh>(m_pCache->transform(wpn.second.model()), m_pCache);
            m->name(wpn.second.name());
            m->position(spawn->position(Space::WORLD) + glm::vec3(0.0f, 0.5f, 0.0f));
            m_pRoot->add(m);
            auto mp = m.get();
            m->on_tick.connect([mp](Freq::Time t){
                mp->rotate(t.s(), glm::vec3(0.0f, 1.0f, 0.0f));
            });
            m_WeaponPickups.push_back(m);
            if(not m_pNet->remote())
                for(auto&& player: m_Players)
                    register_pickup_with_player(m, player.get());
        }
    }
    for(auto&& item: *m_pConfig->meta("items"))
    {
        auto m = item.as<shared_ptr<Meta>>();
        string model = m->at("model", string());
        if(model.empty())
            continue;
        auto spawns = m_pRoot->hook(item.key + string(".*"), Node::Hook::REGEX);
        for(auto&& spawn: spawns)
        {
            auto shape = make_shared<Mesh>(m_pCache->transform(model), m_pCache);
            auto re = m->at("skin", shared_ptr<Meta>());
            if(re)
            {
                if(shape->compositor())
                {
                    auto children = shape->hook_type<Mesh>();
                    for(auto&& c: children)
                    {
                        string oldskin = c->material()->texture()->filename();
                        string skin = boost::regex_replace(
                            oldskin,
                            boost::regex(re->at<string>(0), boost::regex_constants::extended),
                            re->at<string>(1)
                        );
                        if(oldskin != skin){
                            c->fork();
                            c->material(skin, m_pCache);
                        }
                    }
                }else{
                    string oldskin = shape->material()->texture()->filename();
                    string skin = boost::regex_replace(
                        oldskin,
                        boost::regex(re->at<string>(0), boost::regex_constants::extended),
                        re->at<string>(1)
                    );
                    if(oldskin != skin){
                        shape->fork();
                        shape->material(skin, m_pCache);
                    }
                }
            }
            
            shape->name(item.key);
            shape->position(spawn->position(Space::WORLD) + glm::vec3(0.0f, 0.5f, 0.0f));
            m_pRoot->add(shape);
            auto mp = shape.get();
            shape->on_tick.connect([mp](Freq::Time t){
                mp->rotate(t.s(), glm::vec3(0.0f, 1.0f, 0.0f));
            });
            m_ItemPickups.push_back(shape);
            if(not m_pNet->remote())
                for(auto&& player: m_Players)
                    register_pickup_with_player(shape, player.get());
        }
    }
    
    if(m_pNet->server()){
        // Send server info to new clients when they give us their info
        auto net = m_pNet;
        string mapname = m_Map;
        m_pNet->on_info.connect([net,mapname](Packet* packet){
            net->info(
                mapname,
                net->get_object_id_for(packet->guid),
                net->profile(packet->guid)->name(),
                packet->guid
            );
        }); 
    }
    
    if(m_pNet->server())
    {
        auto gamespec = this;
        auto net = m_pNet;
        m_SpawnCon = m_pNet->on_spawn.connect([gamespec,net](Packet* packet){
            auto prof = net->profile(packet->guid);
            uint32_t obj_id;
            try{
                obj_id = net->get_object_id_for(packet->guid);
            }catch(...){
                LOG("no object ID")
                return;
            }

            // do player spawn / gamespec should return null if player can't spawn
            Player* p = gamespec->play(prof);
            if(p)
            {
                net->add_object(obj_id, p->shape());
                p->shape()->config()->set<int>("id", obj_id);
                prof->temp()->set<int>("id", obj_id);

                net->spawn(packet->guid); // broadcast spawn
                //gamespec->server_notify_spawn(packet, true);
            }
            else
            {
                //LOG("no player to spawn");
            }
        });
        m_UpdateCon = m_pNet->on_update.connect(bind(&GameSpec::recv_update, this, std::placeholders::_1));
        m_DoneLoadingCon = m_pNet->on_done_loading.connect(bind(
            &GameSpec::server_notify_spawn, this, std::placeholders::_1, false
        ));
    }
    else if(m_pNet->remote())
    {
        auto gamespec = this;
        auto net = m_pNet;
        m_SpawnCon = m_pNet->on_spawn.connect(bind(&GameSpec::client_spawn, this, std::placeholders::_1));
        m_DespawnCon = m_pNet->on_despawn.connect(bind(&GameSpec::client_despawn, this, std::placeholders::_1));
        m_UpdateCon = m_pNet->on_update.connect(bind(&GameSpec::recv_update, this, std::placeholders::_1));
    }
}

void GameSpec :: enter()
{
    if(m_pNet->remote())
        client_done_loading();
}

Player* GameSpec :: play(shared_ptr<Profile> prof)
{
    // does profile already have a player?
    //LOG("player spawning...");
    
    for(auto&& p: m_Players) 
        if(p->profile() == prof){
            return nullptr;
        }
    
    if(not prof || not prof->dummy())
        prof = m_pSpectator ? m_pSpectator->profile() : prof;
           
    auto win = m_pQor->window();
    //auto console = m_pConsole.get();
    auto player = std::make_shared<Player>(
        m_pState,
        m_pRoot,
        prof,
        m_pQor->resources(),
        m_pPhysics,
        m_pQor->window(),
        m_pQor,
        this,
        m_pNet,
        m_pSpectator ? m_pSpectator->node()->position(Space::WORLD) : glm::vec3(),
        m_LockIf
        //[console]{ return console->input();}
    );
    if(not teleport_to_spawn(player.get()))
        return nullptr;
    
    player->reset();
    
    register_player(player->shared_from_this());
    
    // local?
    if(not prof->dummy()) {
        m_pSpectator = nullptr;
        m_pPlayer = player.get();
        m_pCamera = m_pPlayer->camera();
        m_pOrthoCamera = m_pPlayer->ortho_camera();
    }
    
    on_player_spawn(player.get());
    
    //register_player(player->shared_from_this());
    m_pPhysics->generate(player->shape().get());
    
    //LOG("player spawned");
    return player.get();
}

void GameSpec :: respawn(Player* player)
{
    if(not teleport_to_spawn(player))
        return;
    
    player->reset();
}

bool GameSpec :: teleport_to_spawn(Player* p)
{
    auto spawns = m_pRoot->hook(R"([Ss]pawn.*)", Node::Hook::REGEX);
    if(not spawns.empty())
    {
        auto spawn = spawns[rand() % spawns.size()];
        p->shape()->teleport(spawn->position(Space::WORLD) + glm::vec3(0.0f, 0.6f, 0.0f));
        return true;
    }
    
    // TODO: shouldn't spawn w/o spawn point,
    //       but spawning at origin is fine for testing
    return true;
}

void GameSpec :: despawn(Player* p)
{
    deregister_player(p);
}

void GameSpec :: spectate(shared_ptr<Profile> prof)
{
    m_pSpectator = std::make_shared<Spectator>(
        m_pState,
        m_pRoot,
        m_pPlayer ? m_pPlayer->profile() : prof,
        m_pQor->resources(),
        m_pPhysics,
        m_pQor->window(),
        m_pQor,
        this,
        m_pNet,
        m_pPlayer ? m_pPlayer->shape()->position(Space::WORLD) : glm::vec3(),
        m_LockIf
    );
    if(m_pPlayer){
        deregister_player(m_pPlayer);
        m_pPlayer = nullptr;
    }
    
    m_pCamera = m_pSpectator->camera();
    m_pOrthoCamera = m_pSpectator->ortho_camera();

    on_spectator_spawn(m_pSpectator.get());
    
    //m_pSkyboxCamera->track(m_pCamera);
    //m_pSkyboxCamera->position(glm::vec3(0.0f));
}

void GameSpec :: spawn_local_spectator()
{
    spectate(m_pProfile);
}

void GameSpec :: spawn_local_player()
{
    play(m_pProfile);
}

void GameSpec :: logic(Freq::Time t)
{
    if(m_pSpectator)
        m_pSpectator->logic(t);
    auto players = m_Players;
    for(auto&& player: players)
        player->logic(t);
}

std::shared_ptr<Node> GameSpec :: ortho_root() const
{
    if(m_pSpectator)
        return m_pSpectator->ortho_root();
    else if(m_pPlayer)
        return m_pPlayer->ortho_root();
    else{
        return nullptr;
    }
}

void GameSpec :: client_spawn(Packet* packet)
{
    //LOG("client_spawn(packet)");
    BitStream bs(packet->data, packet->length, false);
    unsigned char c;
    bs.Read(c); // we already know this is ID_SPAWN
    bs.Read(c);
    if(c == NetSpec::OBJ_PLAYER)
    {
        // was this player just spawned, or am i only now hearing about it?
        bool now;
        bs.Read(now);
        
        uint32_t obj_id;
        bs.Read(obj_id);
        m_pNet->reserve(obj_id);

        auto id = m_pProfile->session()->meta()->template at<int>("id");
        if(obj_id == id){
            Player* p = play(m_pProfile);
            if(p)
            {
                //LOGf("my id is %s, and I'm spawning %s", id % obj_id);
                m_pNet->add_object(obj_id, p->shape());
                p->shape()->config()->set<int>("id", obj_id);
                m_pProfile->temp()->set<int>("id", obj_id);
            }
            
        }else{
            // make dummy to mark remote player
            if(not m_pNet->has_object(obj_id))
            {
                //LOGf("my id is %s, and I'm spawning %s", id % obj_id);
                //LOG(to_string(m_pProfile->session()->meta()->template at<int>("id")));
                RakString s;
                bs.Read(s);
                string player_name = s.C_String();
                
                auto prof = m_pProfile->session()->dummy_profile(player_name);
                Player* p = play(prof);
                if(not p){
                    LOG("unable to spawn player");
                    return;
                }
                m_pNet->add_object(obj_id, p->shape());
                p->shape()->config()->set<int>("id", obj_id);
                prof->temp()->set<int>("id", obj_id);
            }
        }
    }
}

// Tell packet's player about all existing players
void GameSpec :: server_notify_spawn(Packet* p, bool now)
{
    //LOG("server_notify_spawn(packet)");
    
    // TODO: notify new player of other players
    auto client_id = m_pNet->get_object_id_for(p->guid);
    for(auto&& obj: m_pNet->nodes())
    {
        //Player* p = obj.second->config()->at<void*>("player", nullptr);
        //if(not p)
        //    return;
        //uint32_t obj_id = p->profile()->temp()->at<int>("id");
        uint32_t obj_id = obj.first;
        if(client_id != obj_id) { // player not matching recver
            LOGf("object id exists: %s", obj.first);
            BitStream bs;
            bs.Write((unsigned char)NetSpec::ID_SPAWN);
            bs.Write((unsigned char)NetSpec::OBJ_PLAYER);
            bs.Write(now);
            bs.Write((uint32_t)obj_id);
            bs.Write(RakString(m_pNet->profile(p->guid)->name().c_str()));
            m_pNet->socket()->Send(
                &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->guid, false
            );
        }
    }
}


void GameSpec :: client_despawn(Packet* packet)
{
    //LOG("client_despawn(packet)");
    BitStream bs(packet->data, packet->length, false);
    unsigned char id;
    bs.Read(id); // we already know this is ID_DESPAWN
    bs.Read(id);
    if(id == NetSpec::OBJ_PLAYER)
    {
        bool kill;
        bs.Read(kill);
        // kill?
        shared_ptr<Node> obj;
        try{
            obj = m_pNet->object(id);
        }catch(const std::out_of_range){
            LOGf("no object of id %s", id);
            return;
        }
        
        if(kill)
        {
            auto player = (Player*)obj->config()->at<void*>("player",nullptr);
            if(not player){
                LOGf("id %s is not a player", id);
                return;
            }
            player->die();
            m_pNet->remove_object(id);
        }
        else
        {
            m_pNet->remove_object(id);
        }
        
        RakString rs;
        bs.Read(rs);
        string s = rs.C_String();
        if(not s.empty()){
            LOG(s); // kill message, if any?
        }
    }
    else{
        auto obj = m_pNet->object(id);
        obj->detach();
        m_pNet->remove_object(id);
    }
}

void GameSpec :: server_despawn(Player* p)
{
    //LOG("server_despawn()");
    BitStream bs;
    bs.Write((unsigned char)NetSpec::ID_DESPAWN);
    bs.Write((unsigned char)NetSpec::OBJ_PLAYER);
    bs.Write((uint32_t)p->shape()->config()->at<int>("id"));
    bool killed = true;
    bs.Write(killed);
    RakString rs(p->death_msg().c_str());
    bs.Write(rs);
   
    m_pNet->socket()->Send(
        &bs,
        HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_RAKNET_GUID, true
    );
}

void GameSpec :: recv_update(Packet* p)
{
    BitStream bs(p->data, p->length, false);
    unsigned char c;
    bs.Read(c); // ID_UPDATE
    bs.Read(c); //OBJ_PLAYER
    uint32_t id;
    bs.Read(id);
    
    shared_ptr<Node> obj;
    try{
        obj = m_pNet->object(id);
    }catch(const std::out_of_range&){
        //LOGf("no object of id %s", id); // TODO: BUG
        return;
    }

    if(c == NetSpec::OBJ_PLAYER)
    {
        auto player = (Player*)obj->config()->at<void*>("player",nullptr);
        if(not player){
            LOG("obj has no attribute player")
            return;
        }
        if(player->local()){
            //LOG("net transform")
            return; // TODO: set net transform
        }
        
        mat4 m;
        float* mp = glm::value_ptr(m);
        for(int i=0;i<16;++i)
            bs.Read(mp[i]);

        //obj->teleport(m);
        player->unpack_transform(m);
    }
}

void GameSpec :: send_update(Player* p)
{
    BitStream bs;
    bs.Write((unsigned char)NetSpec::ID_UPDATE);
    bs.Write((unsigned char)NetSpec::OBJ_PLAYER);
    auto id = (uint32_t)p->shape()->config()->at<int>("id");
    bs.Write(id);
    mat4 m(p->pack_transform());
    float* mp = glm::value_ptr(m);
    for(int i=0;i<16;++i)
        bs.Write(mp[i]);
    m_pNet->socket()->Send(
        &bs,
        HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, UNASSIGNED_RAKNET_GUID, true
    );
}

void GameSpec :: client_done_loading()
{
    //LOG("client_done_loading")
    BitStream bs;
    bs.Write((unsigned char)NetSpec::ID_DONE_LOADING);
    m_pNet->socket()->Send(
        &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_RAKNET_GUID, true
    );
}


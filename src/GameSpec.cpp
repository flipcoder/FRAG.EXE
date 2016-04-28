#include "Qor/Mesh.h"
#include "Qor/Node.h"
#include "Player.h"
#include "Spectator.h"
#include "Qor/Qor.h"
#include "Game.h"
#include <boost/regex.hpp>
#include "Qor/Profile.h"
using namespace std;
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
}

Player* GameSpec :: play(shared_ptr<Profile> prof)
{
    // does profile already have a player?
    for(auto&& p: m_Players) 
        if(p->profile() == prof)
            return nullptr;
    
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
    
    return player.get();
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

void GameSpec :: spawn(Packet* packet)
{
    LOG("spawn(packet)");
    BitStream bs(packet->data, packet->length, false);
    unsigned char id;
    bs.Read(id); // we already know this is ID_SPAWN
    bs.Read(id);
    if(id == NetSpec::OBJ_PLAYER)
    {
        // was this player just spawned, or am i only now hearing about it?
        bool spawn_now;
        bs.Read(spawn_now);
        
        uint32_t obj_id;
        bs.Read(obj_id);
        m_pNet->reserve(obj_id);

        LOG(to_string(obj_id));
        LOG(to_string(m_pProfile->session()->meta()->template at<int>("id")));
        if(obj_id == m_pProfile->session()->meta()->template at<int>("id")){
            // spawn player?
            LOG("spawn player");
            Player* p = play(m_pProfile);
            m_pNet->add_object(obj_id, p->shape());
            p->shape()->config()->set("id", obj_id);
            m_pProfile->temp()->set("id", obj_id);
            
        }else{
            // make dummy to mark remote player
            RakString s;
            bs.Read(s);
            string player_name = s.C_String();
            
            auto prof = m_pProfile->session()->dummy_profile(player_name);
            Player* p = play(prof);
            m_pNet->add_object(obj_id, p->shape());
            p->shape()->config()->set("id", obj_id);
            prof->temp()->set("id", obj_id);
        }
    }
}


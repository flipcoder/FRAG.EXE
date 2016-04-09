#include "Qor/Mesh.h"
#include "Qor/Node.h"
#include "Player.h"
using namespace std;

GameSpec :: GameSpec(std::string fn, Cache<Resource, std::string>* cache,
    Node* root, BasicPartitioner* part
):
    m_pConfig(make_shared<Meta>(cache->transform(fn))),
    m_pCache(cache),
    m_WeaponSpec(m_pConfig->meta("weapons")),
    m_pRoot(root),
    m_pPartitioner(part)
{}

void GameSpec :: register_player(Player* p)
{
    m_Players.push_back(p);
    for(auto&& wpn: m_WeaponPickups)
        register_pickup_with_player(wpn, p);
    for(auto&& item: m_ItemPickups)
        register_pickup_with_player(item, p);
}

void GameSpec :: deregister_player(Player* p)
{
    kit::remove(m_Players, p);
}

void GameSpec :: register_pickup_with_player(std::shared_ptr<Mesh> item, Player* player)
{
    auto part = m_pPartitioner;
    m_pPartitioner->on_touch(item, player->mesh(), [part, player](Node* a, Node* b){
        assert(a && b);
        if(not a->attached() && not a->detaching())
            return;
        // spawn item with spawn
        a->config()->set<string>("name", a->name());
        player->give(a->config());
        a->safe_detach();
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
            for(auto&& player: m_Players)
                register_pickup_with_player(m, player);
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
            auto m = make_shared<Mesh>(m_pCache->transform(model), m_pCache);
            m->name(item.key);
            m->position(spawn->position(Space::WORLD) + glm::vec3(0.0f, 0.5f, 0.0f));
            m_pRoot->add(m);
            auto mp = m.get();
            m->on_tick.connect([mp](Freq::Time t){
                mp->rotate(t.s(), glm::vec3(0.0f, 1.0f, 0.0f));
            });
            m_ItemPickups.push_back(m);
            for(auto&& player: m_Players)
                register_pickup_with_player(m, player);
        }
    }
}


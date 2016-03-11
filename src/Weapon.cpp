#include "Weapon.h"
using namespace std;

WeaponSpecEntry :: WeaponSpecEntry(
    string name,
    const shared_ptr<Meta>& cfg,
    WeaponSpec* spec
):
    m_Name(name),
    m_pConfig(cfg),
    m_pSpec(spec),
    m_Model(m_pConfig->at<string>("model", string())),
    m_Projectile(m_pConfig->at<string>("projectile", string())),
    m_Sound(m_pConfig->at<string>("sound", string())),
    m_Slot(m_pConfig->at<int>("slot")),
    m_Bias(m_pConfig->at<int>("bias",0)),
    m_Burst(m_pConfig->at<int>("burst",1)),
    m_Spread(m_pConfig->at<double>("spread",0.0f)),
    m_Delay(Freq::Time::seconds(m_pConfig->at<double>("delay",1.0f))),
    m_bScope(m_pConfig->at<bool>("scope",false))
{
    auto m = m_pConfig->at<shared_ptr<Meta>>("pos", make_shared<Meta>(MetaFormat::JSON, "[0.0,0.0,0.0]"));
    m_ViewModelPos = glm::vec3(
        m->at<double>(0), m->at<double>(1), m->at<double>(2)
    );
    m = m_pConfig->at<shared_ptr<Meta>>("zoomed_pos", make_shared<Meta>(MetaFormat::JSON, "[0.0,0.0,0.0]"));
    m_ViewModelZoomedPos = glm::vec3(
        m->at<double>(0), m->at<double>(1), m->at<double>(2)
    );
}

WeaponSpec :: WeaponSpec(const shared_ptr<Meta>& cfg):
    m_pConfig(cfg)
{
    for(auto&& e: *cfg){
        m_Entries.emplace(e.key, e.as<shared_ptr<Meta>>(), this);
    }
}

WeaponStash :: WeaponStash(WeaponSpec* spec):
    m_pSpec(spec)
{
    m_Slots.reserve(10);
    for(int i=0; i<10; ++i)
        m_Slots.emplace_back();
}

void WeaponStash :: give_all()
{
    for(auto&& e: *m_pSpec)
        give(&e.second);
}

bool WeaponStash :: give(WeaponSpecEntry* spec)
{
    auto& slot = m_Slots[spec->slot()];
    if(std::find_if(ENTIRE(slot), [spec](Weapon& w){
        return w.spec()->name() == spec->name();
    }) != slot.end())
    {
        // TODO: attempt to merge
        return false;
    }
    m_Slots[spec->slot()].emplace_back(spec);
    return true;
}

bool WeaponStash :: next(int delta)
{
    assert(delta);
    
    // make sure we have an active weapon
    if(not m_pActive)
    {
        for(int i=0;i<10;++i)
            try{
                m_pActive = &m_Slots[i].at(0);
                break;
            }catch(const out_of_range&){}
        delta = delta - kit::sign(delta); // this counts as one switch
    }
    
    int active_slot = m_pActive->spec()->slot();
    bool r = false;
    // flip through weapons in direction of delta
    while(delta)
    {
        auto dir = kit::sign(delta);
        Weapon* next = next_in_slot(m_pActive, dir);
        if(not next)
        {
            int i = 0;
            do{
                active_slot = (active_slot + dir) % 10;
                if(active_slot == -1)
                    active_slot = 9;
                ++i;
                if(i>10) {
                    m_pActive = nullptr; // no weapons?
                    return true; // technically, this was a successful switch
                }
            } while(m_Slots[active_slot].empty());
            
            // approach from what side?
            if(dir==1)
                m_pActive = &m_Slots[active_slot][0];
            else
                m_pActive = &m_Slots[active_slot][m_Slots[active_slot].size()-1];
            r = true;
        }
        delta = delta - dir; // delta itr goes towards 0
    }
    return r;
}

Weapon* WeaponStash :: next_in_slot(Weapon* active, int dir)
{
    assert(dir);
    // get active weapon's slot
    //auto&& slot = m_Slots[active->spec()->slot()];
    //try{
    //    // find our active weapon in the list
    //    auto itr = std::find_if(ENTIRE(slot), [active](const Weapon& w){
    //        return &w == active;
    //    });
    //    // try to return the next weapon in the direction dir
    //    return &*(itr + dir);
    //}catch(const std::out_of_range&){}
    // no more weapons in slot, return nullptr
    return nullptr;
}

bool WeaponStash :: slot(int num)
{
    if(num == m_pActive->spec()->slot())
        return false; // TODO: next_in_slot w/ wrapping
    try{
        m_pActive = &m_Slots[num].at(0);
        return true;
    }catch(const out_of_range&){}
    return false;
}


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
    m_bGravity(m_pConfig->at<bool>("gravity", false)),
    m_Sound(m_pConfig->at<string>("sound", string())),
    m_Slot(m_pConfig->at<int>("slot")),
    m_Bias(m_pConfig->at<int>("bias",0)),
    m_Burst(m_pConfig->at<int>("burst",1)),
    m_Spread(m_pConfig->at<double>("spread",0.0f)),
    m_Delay(Freq::Time::seconds(m_pConfig->at<double>("delay",1.0f))),
    m_bScope(m_pConfig->at<bool>("scope",false)),
    m_Speed(m_pConfig->at<double>("speed",0.0f)),
    m_Ammo(m_pConfig->at<int>("ammo",-1)), // -1 == unlimited ammo
    m_Clip(m_pConfig->at<int>("clip",0)), // = no reload needed
    m_Damage(m_pConfig->at<int>("damage",1)),
    m_Radius(m_pConfig->at<double>("radius",1.0))
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
    clear();
}

void WeaponStash :: clear()
{
    m_pActive = nullptr;
    m_Slots.clear();
    m_Slots.reserve(10);
    for(int i=0; i<10; ++i)
        m_Slots.emplace_back();
}

void WeaponStash :: give_all()
{
    for(auto&& e: *m_pSpec)
        give(&e.second);
}

bool WeaponStash :: fill_all()
{
    for(auto&& slot: m_Slots)
        for(auto&& wpn: slot)
            wpn->fill();
    return true;
}


bool WeaponStash :: give(const std::shared_ptr<Meta>& config)
{
    // attempt to parse spec as a weapon
    string name;
    try{
        name = config->at<string>("name");
    }catch(...){
        return false;
    }
    for(auto&& e: *m_pSpec)
    {
        if(name == e.second.name()) {
            // merge details
            return give(&e.second, config);
        }
    }
    return false;
}

bool WeaponStash :: give(std::string name)
{
    auto m = make_shared<Meta>();
    m->set<string>("name", name);
    return give(m);
}

bool WeaponStash :: give(WeaponSpecEntry* spec, std::shared_ptr<Meta> config)
{
    auto& slot = m_Slots[spec->slot()];
    auto wpn = std::find_if(ENTIRE(slot), [spec](const shared_ptr<Weapon>& w){
        return w->spec()->name() == spec->name();
    });
    if(wpn != slot.end())
    {
        // TODO: attempt to merge
        //return wpn->merge(config);
        return (*wpn)->fill();
    }
    slot.emplace_back(make_shared<Weapon>(spec, config));
    sort_slot(slot);
    
    return true;
}

void WeaponStash :: sort_slot(std::vector<std::shared_ptr<Weapon>>& slot)
{
    std::sort(ENTIRE(slot), [](shared_ptr<Weapon> a, shared_ptr<Weapon> b){
        return a->spec()->bias() < b->spec()->bias();
    });
}

unsigned WeaponStash :: weapon_count() const
{
    unsigned r = 0;
    for(auto&& slot: m_Slots)
        r += slot.size();
    return r;
}

bool WeaponStash :: next(int delta)
{
    assert(delta);
    
    // make sure we have an active weapon
    if(not m_pActive)
    {
        for(int i=0;i<10;++i)
            try{
                m_pActive = m_Slots[i].at(0).get();
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
            // don't switch if only slot
            if(weapon_count() == 1)
                return false;
            
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
                m_pActive = m_Slots[active_slot][0].get();
            else
                m_pActive = m_Slots[active_slot][m_Slots[active_slot].size()-1].get();
            r = true;
        } else {
            m_pActive = next;
            r = true;
        }
        delta = delta - dir; // delta itr goes towards 0
    }
    return r;
}

Weapon* WeaponStash :: next_in_slot(Weapon* active, int dir, bool wrap)
{
    // get active weapon's slot
    auto&& slot = m_Slots[active->spec()->slot()];
    try{
        // find our active weapon in the list
        auto itr = std::find_if(ENTIRE(slot), [active](const shared_ptr<Weapon>& w){
            return w.get() == active;
        });
        unsigned idx = std::distance(slot.begin(), itr);
        // try to return the next weapon in the direction dir
        if(wrap)
            return slot.at((idx + dir) % slot.size()).get();
        else
            return slot.at(idx + dir).get();
    }catch(const std::out_of_range&){}
    // no more weapons in slot, return nullptr
    return nullptr;
}

bool WeaponStash :: slot(int num)
{
    auto last_active = m_pActive;
    if(m_pActive && num == m_pActive->spec()->slot())
    {
        auto active = next_in_slot(m_pActive, 1, true);
        if(active)
            m_pActive = active;
        return active && active != last_active;
    }
    try{
        m_pActive = m_Slots[num].at(0).get();
        return true;
    }catch(const out_of_range&){}
    return false;
}

bool WeaponStash :: slot(std::string s)
{
    for(auto&& slot: m_Slots)
    {
        for(auto&& wpn: slot){
            if(wpn->spec()->name() == s){
                m_pActive = wpn.get();
                return true;
            }
        }
    }
    return false;
}

Weapon :: Weapon(WeaponSpecEntry* spec, std::shared_ptr<Meta> config):
    m_pSpec(spec)
    //m_Clip(spec->clip())
{
    if(config && not config->empty())
    {
        // deserialize config, otherwise do defaults
        fill();
    }
    else
    {
        // default is to fill
        fill();
    }
}

bool Weapon :: fill()
{
   bool success =
       m_Clip != m_pSpec->clip() ||
       m_Ammo != m_pSpec->ammo();
   m_Clip = m_pSpec->clip();
   m_Ammo = m_pSpec->ammo();
   return success;
}

bool Weapon :: can_reload() const
{
    return m_Ammo == -1 ||
        (
            m_pSpec->clip() > 0 && // no reload needed
            m_Ammo > 0 &&
            m_Clip != m_pSpec->clip()
        ); // clip full?
}

bool Weapon :: reload()
{
    if(not can_reload())
        return false;
    int needed = m_pSpec->clip() - m_Clip;
    int transfer = std::min<int>(needed, m_Ammo);
    m_Clip = m_Clip + transfer;
    if(m_Ammo >= 0) // limited ammo?
        m_Ammo = m_Ammo - transfer;
    return true;
}

int Weapon :: fire()
{
    int burst;
    if(m_pSpec->clip() == 0) { // no reload needed
        burst = std::min<int>(m_Ammo, m_pSpec->burst());
        m_Ammo = m_Ammo - burst;
    }
    else
    {
        burst = std::min<int>(m_Clip, m_pSpec->burst());
        m_Clip = m_Clip - burst;
    }
    return burst;
}

WeaponSpecEntry* WeaponSpec :: weapon(std::string name)
{
    Weapon* w = nullptr;
    for(auto&& wpn: m_Entries)
    {
        if(wpn.second.name() == name)
            return &wpn.second;
    }
    return nullptr;
}


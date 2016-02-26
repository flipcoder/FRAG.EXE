#ifndef _WEAPON_H
#define _WEAPON_H

#include "Qor/kit/meta/meta.h"
#include "Qor/Common.h"
#include "Qor/kit/freq/freq.h"
#include <glm/glm.hpp>

class WeaponSpec;
class WeaponSpecEntry
{
    public:
        WeaponSpecEntry() {}
        WeaponSpecEntry(std::string name, const std::shared_ptr<Meta>& cfg, WeaponSpec* spec);
        ~WeaponSpecEntry() = default;
        WeaponSpecEntry(WeaponSpecEntry&&) = default;
        WeaponSpecEntry(const WeaponSpecEntry&) = default;
        WeaponSpecEntry& operator=(const WeaponSpecEntry&) = default;
        WeaponSpecEntry& operator=(WeaponSpecEntry&&) = default;

        std::string name() const { return m_Name; }
        int slot() const { return m_Slot; }
        WeaponSpec* spec() const { return m_pSpec; }
        std::string model() const { return m_Model; }
        std::string sound() const { return m_Sound; }
        glm::vec3 viewmodel_pos() const { return m_ViewModelPos;}
        glm::vec3 viewmodel_zoomed_pos() const { return m_ViewModelZoomedPos;}
        int burst() const { return m_Burst; }
        float spread() const { return m_Spread; }
        Freq::Time delay() const { return m_Delay; }
        
    private:
        std::string m_Name;
        std::shared_ptr<Meta> m_pConfig;
        WeaponSpec* m_pSpec;
        std::string m_Model;
        std::string m_Sound;
        int m_Slot = 0;
        int m_Bias = 0;
        int m_Burst = 0;
        float m_Spread = 0.0f;
        Freq::Time m_Delay;
        glm::vec3 m_ViewModelPos;
        glm::vec3 m_ViewModelZoomedPos;
};

class WeaponSpec
{
    public:
        WeaponSpec(const std::shared_ptr<Meta>& cfg);
        
        typedef typename kit::index<WeaponSpecEntry>::const_iterator
            const_iterator;
        typedef typename kit::index<WeaponSpecEntry>::iterator
            iterator;
        iterator begin() { return m_Entries.begin(); }
        iterator end() { return m_Entries.end(); }
        const_iterator begin() const { return m_Entries.begin(); }
        const_iterator end() const { return m_Entries.end(); }
        const_iterator cbegin() const { return m_Entries.begin(); }
        const_iterator cend() const { return m_Entries.end(); }

    private:
        std::shared_ptr<Meta> m_pConfig;
        kit::index<WeaponSpecEntry> m_Entries;
};

class Weapon
{
    public:
        Weapon(WeaponSpecEntry* spec):
            m_pSpec(spec)
        {}
        WeaponSpecEntry* spec() { return m_pSpec; }
    private:
        WeaponSpecEntry* m_pSpec;
};

class WeaponStash
{
    public:
        WeaponStash(WeaponSpec* spec);

        void give_all();
        bool give(WeaponSpecEntry* spec);

        bool next(int delta=1);
        Weapon* next_in_slot(Weapon* active, int dir);

        Weapon* active() { return m_pActive; }

    private:
        std::vector<std::vector<Weapon>> m_Slots;
        WeaponSpec* m_pSpec;
        Weapon* m_pActive = nullptr;
};

#endif


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
        std::string projectile() const { return m_Projectile; }
        std::string sound() const { return m_Sound; }
        glm::vec3 viewmodel_pos() const { return m_ViewModelPos;}
        glm::vec3 viewmodel_zoomed_pos() const { return m_ViewModelZoomedPos;}
        int burst() const { return m_Burst; }
        float spread() const { return m_Spread; }
        Freq::Time delay() const { return m_Delay; }
        bool scope() const { return m_bScope; }
        bool gravity() const { return m_bGravity; }
        float speed() const { return m_Speed; }
        int bias() const { return m_Bias; }
        int clip() const { return m_Clip; }
        int ammo() const { return m_Ammo; }
        int damage() const { return m_Damage; }
        float radius() const { return m_Radius; }
        
    private:
        
        std::string m_Name;
        std::shared_ptr<Meta> m_pConfig;
        WeaponSpec* m_pSpec;
        std::string m_Model;
        std::string m_Projectile;
        bool m_bGravity = false;
        std::string m_Sound;
        int m_Slot = 0;
        int m_Bias = 0;
        int m_Burst = 0;
        float m_Spread = 0.0f;
        Freq::Time m_Delay;
        glm::vec3 m_ViewModelPos;
        glm::vec3 m_ViewModelZoomedPos;
        bool m_bScope = false;
        float m_Speed = 0.0f;
        int m_Ammo = 0;
        int m_Clip = 0;
        int m_Damage = 1;
        float m_Radius = 1.0f;
};

class WeaponSpec
{
    public:
        WeaponSpec(const std::shared_ptr<Meta>& cfg);

        WeaponSpecEntry* weapon(std::string n);
        
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
        Weapon(WeaponSpecEntry* spec, std::shared_ptr<Meta> config = nullptr);
        WeaponSpecEntry* spec() { return m_pSpec; }
        
        bool fill();
        int fire();
        bool reload();
        bool can_reload() const;
        //kit::reactive<int>& ammo() { return m_Ammo; }
        //kit::reactive<int>& clip() { return m_Clip; }
        int ammo() const { return m_Ammo; }
        int clip() const { return m_Clip; }
        
    private:
        WeaponSpecEntry* m_pSpec;
        int m_Ammo = 0;
        int m_Clip = 0;
        //kit::reactive<int> m_Ammo;
        //kit::reactive<int> m_Clip;
};

class WeaponStash
{
    public:
        WeaponStash(WeaponSpec* spec);

        void give_all();
        bool give(std::string name);
        bool give(const std::shared_ptr<Meta>& config);
        bool give(WeaponSpecEntry* spec, std::shared_ptr<Meta> config = nullptr);
        bool fill_all();

        bool next(int delta=1);
        bool slot(int num);
        bool slot(std::string name);
        Weapon* next_in_slot(Weapon* active, int dir, bool wrap = false);

        Weapon* active() { return m_pActive; }

        void sort_slot(std::vector<std::shared_ptr<Weapon>>& slot);

        unsigned weapon_count() const;
        
    private:
        
        std::vector<std::vector<std::shared_ptr<Weapon>>> m_Slots;
        WeaponSpec* m_pSpec;
        Weapon* m_pActive = nullptr;
};

#endif


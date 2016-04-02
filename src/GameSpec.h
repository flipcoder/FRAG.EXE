#ifndef _GAMESPEC_H
#define _GAMESPEC_H

#include <memory>
#include <string>
#include "Qor/kit/meta/meta.h"
#include "Qor/kit/cache/cache.h"
#include "Qor/Resource.h"
#include "Weapon.h"
class Node;

class GameSpec
{
    public:
        GameSpec(std::string fn, Cache<Resource, std::string>* cache, Node* root);
        WeaponSpec* weapons() { return &m_WeaponSpec; }

        void setup();
        
    private:
        std::shared_ptr<Meta> m_pConfig;
        Cache<Resource, std::string>* m_pCache;
        WeaponSpec m_WeaponSpec;
        Node* m_pRoot;
};

#endif


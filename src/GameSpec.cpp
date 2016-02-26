#include "GameSpec.h"
#include "Qor/Mesh.h"
using namespace std;

GameSpec :: GameSpec(std::string fn, Cache<Resource, std::string>* cache):
    m_pConfig(make_shared<Meta>(cache->transform(fn))),
    m_pCache(cache),
    m_WeaponSpec(m_pConfig->meta("weapons"))
{
    for(auto&& wpn: m_WeaponSpec)
        make_shared<Mesh>(cache->transform(wpn.second.model()), cache);
}


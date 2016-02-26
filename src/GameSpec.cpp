#include "GameSpec.h"
using namespace std;

GameSpec :: GameSpec(std::string fn, Cache<Resource, std::string>* cache):
    m_pConfig(make_shared<Meta>(cache->transform(fn))),
    m_pCache(cache),
    m_WeaponSpec(m_pConfig->meta("weapons"))
{
    
}


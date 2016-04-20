#include "NetSpec.h"
using namespace std;

NetSpec :: NetSpec(Qor* engine, bool server, int connections):
    m_pNet(make_shared<Net>(engine,server,connections))
{}

NetSpec :: ~NetSpec() {}

void NetSpec :: logic(Freq::Time t)
{
    m_pNet->logic(t);
}


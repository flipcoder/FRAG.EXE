#include "Pregame.h"
#include "Qor/BasicPartitioner.h"
#include "Qor/Input.h"
#include "Qor/Qor.h"
#include <glm/glm.hpp>
#include <cstdlib>
#include <chrono>
#include <thread>
using namespace std;
using namespace glm;
using namespace RakNet;

Pregame :: Pregame(Qor* engine):
    m_pQor(engine),
    m_pInput(engine->input()),
    m_pRoot(make_shared<Node>()),
    m_pPipeline(engine->pipeline())
{}

void Pregame :: preload()
{
    m_pCamera = make_shared<Camera>(m_pQor->resources(), m_pQor->window());
    m_pConsole = make_shared<Console>(
        m_pQor->interpreter(),
        m_pQor->window(),
        m_pQor->input(),
        m_pQor->session()->active_profile(0)->controller().get(),
        m_pQor->resources()
    );
    m_pRoot->add(m_pCamera);
    m_pRoot->add(m_pConsole);
    
    if(m_pQor->args().has('d', "dedicated")||
       m_pQor->args().has('s', "server"))
        m_bServer = true;
}

Pregame :: ~Pregame()
{
    m_pPipeline->partitioner()->clear();
}

void Pregame :: enter()
{
    auto net = make_shared<NetSpec>(m_pQor, m_bServer);
    m_pQor->session()->module("net", net);
    m_pNet = net.get();

    if(m_pNet->remote()){
        // TODO: wait for game data?
        auto qor = m_pQor;
        auto net = m_pNet;
        string name = m_pQor->session()->active_profile(0)->name();
        m_ConnectCon = m_pNet->net()->on_connect.connect([qor,net,name](RakNet::Packet*){
            net->info(name);
        });
        m_InfoCon = m_pNet->on_info.connect([qor,net](RakNet::Packet* p){
            BitStream bs(p->data, p->length, false);
            RakString rs;
            unsigned char id;
            bs.Read(id); // id (ID_INFO)
            bs.Read(rs); // map name
            uint32_t self_id, self_guid;
            bs.Read(self_id); // self object id
            LOGf("self id %s", to_string(self_id));
            net->reserve(self_id); // reserve object id for self
            std::string map = rs.C_String();
            //bs.Read(self_guid);
            bs.Read(rs); // new name
            std::string new_name = rs.C_String();
            qor->session()->meta()->set("map",map);
            qor->session()->meta()->set<int>("id",self_id);
            qor->session()->meta()->set<int>("guid",self_guid);
            qor->session()->active_profile(0)->name(new_name);
            LOGf("map: %s", map);
            qor->change_state("game");
        });
    }

    if(m_pNet->server() || m_pNet->local())
        m_pQor->change_state("game");
    
    m_pCamera->ortho();
    m_pPipeline->winding(true);
    m_pPipeline->bg_color(Color::black());
    m_pInput->relative_mouse(false);
}

void Pregame :: logic(Freq::Time t)
{
    m_pNet->logic(t);
    
    if(m_pInput->key(SDLK_ESCAPE))
        m_pQor->quit();

    m_pRoot->logic(t);
}

void Pregame :: render() const
{
    m_pPipeline->render(m_pRoot.get(), m_pCamera.get());
}


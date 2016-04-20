#ifndef NETSPEC_H
#define NETSPEC_H

#include "Qor/Session.h"
#include "Qor/Net.h"

class NetSpec:
    public Session::IModule
{
    public:
        NetSpec(Qor* engine, bool server, int connections = 8);
        virtual ~NetSpec();
        virtual void logic(Freq::Time t) override;

    private:
        std::shared_ptr<Net> m_pNet;
};

#endif


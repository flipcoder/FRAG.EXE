#ifndef _PREGAMESTATE_H
#define _PREGAMESTATE_H

#include "Qor/Node.h"
#include "Qor/State.h"
#include "Qor/Input.h"
#include "Qor/Camera.h"
#include "Qor/Pipeline.h"
#include "Qor/Mesh.h"
#include "Qor/Console.h"
#include "NetSpec.h"

class Qor;

class Pregame:
    public State
{
    public:
        
        Pregame(Qor* engine);
        virtual ~Pregame();

        virtual void preload() override;
        virtual void enter() override;
        virtual void logic(Freq::Time t) override;
        virtual void render() const override;
        virtual bool needs_load() const override {
            return true;
        }

    private:
        
        Qor* m_pQor = nullptr;
        Input* m_pInput = nullptr;
        Pipeline* m_pPipeline = nullptr;

        std::shared_ptr<Node> m_pRoot;
        std::shared_ptr<Console> m_pConsole;
        std::shared_ptr<Camera> m_pCamera;
        NetSpec* m_pNet;

        bool m_bServer = false;

        boost::signals2::scoped_connection m_InfoCon;
        boost::signals2::scoped_connection m_ConnectCon;
};

#endif



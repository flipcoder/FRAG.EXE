#ifndef _MENUSCREEN_H_BIFJJEN3
#define _MENUSCREEN_H_BIFJJEN3

#include "Qor/State.h"
#include "Qor/Input.h"
#include "Qor/Camera.h"
#include "Qor/Pipeline.h"
#include "Qor/Mesh.h"
#include "Qor/Sound.h"
#include "Qor/Canvas.h"
#include "Qor/Menu.h"

class Qor;
class ResourceCache;

class MenuScreen:
    public State
{
    public:
        
        MenuScreen(Qor* engine);
        virtual ~MenuScreen();

        virtual void enter() override;
        virtual void preload() override;
        virtual void logic(Freq::Time t) override;
        virtual void render() const override;
        virtual bool needs_load() const override {
            return true;
        }

        virtual Pipeline* pipeline() {
            return m_pPipeline;
        }
        virtual const Pipeline* pipeline() const {
            return m_pPipeline;
        }
        
        virtual std::shared_ptr<Node> root() override {
            return m_pRoot;
        }
        virtual std::shared_ptr<const Node> root() const override {
            return m_pRoot;
        }
        
        virtual std::shared_ptr<Node> camera() override {
            return m_pCamera;
        }
        virtual std::shared_ptr<const Node> camera() const override {
            return m_pCamera;
        }

        BasicPartitioner* partitioner() {
            return m_pPartitioner;
        }

    private:

        Qor* m_pQor = nullptr;
        Input* m_pInput = nullptr;
        Controller* m_pController = nullptr;
        BasicPartitioner* m_pPartitioner = nullptr;
        Pipeline* m_pPipeline = nullptr;
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<Sound> m_pMusic;
        ResourceCache* m_pResources = nullptr;
        kit::reactive<Color> m_Ambient;

        std::shared_ptr<Node> m_pRoot;

        Freq::Timeline m_MenuScreenTime;

        std::shared_ptr<Canvas> m_pCanvas;
        
        std::shared_ptr<MenuGUI> m_pMenuGUI;
        MenuContext m_MenuContext;
        Menu m_MainMenu;

        float m_Fade = 1.0f;
};

#endif


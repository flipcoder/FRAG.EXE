#ifndef _GAMESTATE_H_BIFJJEN3
#define _GAMESTATE_H_BIFJJEN3

#include "Qor/State.h"
#include "Qor/Input.h"
#include "Qor/Camera.h"
#include "Qor/Pipeline.h"
#include "Qor/Mesh.h"
#include "Qor/Scene.h"
#include "Qor/Interpreter.h"
#include "Qor/Physics.h"
//#include "Qor/BasicPhysics.h"
#include "Qor/Sprite.h"
#include "Qor/ViewModel.h"
#include "Qor/Console.h"
#include "Player.h"
#include "Spectator.h"
#include "GameSpec.h"

class Qor;

class GameState:
    public State
{
    public:
        
        GameState(Qor* engine);
        virtual ~GameState();

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

        virtual void camera(const std::shared_ptr<Node>& camera) override {
            m_pCamera = std::dynamic_pointer_cast<Camera>(camera);
        }
        
        virtual Physics* physics() override {
            return m_pPhysics.get();
        }
        virtual Physics* physics() const override {
            return m_pPhysics.get();
        }

        BasicPartitioner* partitioner() {
            return m_pPartitioner;
        }

        Freq::Timeline* timeline() { return &m_GameTime; }
        
        void play();
        void spectate();
        bool respawn(Player* p);
        void despawn(Player* p);
        
    private:

        //void decal(glm::vec3 contact, glm::vec3 normal, glm::vec3 up, float offset);
        
        Qor* m_pQor = nullptr;
        Input* m_pInput = nullptr;
        BasicPartitioner* m_pPartitioner = nullptr;
        Pipeline* m_pPipeline = nullptr;

        std::shared_ptr<Console> m_pConsole;
        std::shared_ptr<Node> m_pRoot;
        std::shared_ptr<Node> m_pOrthoRoot;
        std::shared_ptr<Node> m_pConsoleRoot;
        std::shared_ptr<Node> m_pSkyboxRoot;
        Interpreter* m_pInterpreter;
        std::shared_ptr<Interpreter::Context> m_pScript;
        std::unique_ptr<Player> m_pPlayer;
        std::unique_ptr<Spectator> m_pSpectator;
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<Camera> m_pOrthoCamera;
        std::shared_ptr<Camera> m_pSkyboxCamera;
        std::shared_ptr<Camera> m_pConsoleCamera;
        std::shared_ptr<Physics> m_pPhysics;
        std::shared_ptr<ViewModel> m_pViewModel;
        std::shared_ptr<Controller> m_pController;
        std::shared_ptr<Scene> m_pScene;

        Freq::Timeline m_GameTime;
        GameSpec m_GameSpec;

        unsigned m_Shader = ~0u;
        unsigned m_ColorShader = ~0u;
};

#endif


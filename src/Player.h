#ifndef _PLAYER_H_W5TRRMBU
#define _PLAYER_H_W5TRRMBU

#include "Qor/PlayerInterface3D.h"
#include "Qor/Camera.h"
#include "Qor/ViewModel.h"
#include "Qor/Mesh.h"
#include "Qor/Input.h"
#include "Qor/Window.h"

class Qor;

class Player:
    public IRealtime
{
    public:
        Player(
            Node* root,
            std::shared_ptr<Controller> controller,
            Cache<Resource, std::string>* cache,
            Physics* physics,
            Window* window,
            Qor* engine,
            std::function<bool()> lock_if = std::function<bool()>()
        );
        void logic(Freq::Time t);

        Camera* camera() { return m_pCamera.get(); }

        bool can_jump() const;
        
    private:
        
        void decal(glm::vec3 contact, glm::vec3 normal, glm::vec3 up, float offset);
        
        Node* m_pRoot;
        std::shared_ptr<Mesh> m_pPlayerMesh;
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<ViewModel> m_pViewModel;
        std::shared_ptr<Controller> m_pController;
        std::shared_ptr<PlayerInterface3D> m_pInterface;
        Cache<Resource, std::string>* m_pCache;
        Physics* m_pPhysics;
        Window* m_pWindow;
        Qor* m_pQor;
        std::function<bool()> m_LockIf;

        std::shared_ptr<ITexture> m_pDecal;
        std::deque<std::shared_ptr<Mesh>> m_Decals;
        static const unsigned MAX_DECALS;

};

#endif


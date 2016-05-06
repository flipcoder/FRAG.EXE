#ifndef _HUD_H
#define _HUD_H

#include "Qor/Window.h"
#include "Qor/Canvas.h"
#include "Qor/Input.h"
#include "Qor/Interpreter.h"

class Player;

class HUD:
    public Node
{
    public:
        HUD(
            Player* player,
            Window* window,
            Input* input,
            Cache<Resource,std::string>* cache
        );
        virtual ~HUD() {}

        virtual void logic_self(Freq::Time) override;

        bool input() const {
            return m_bInput;
        }

        void hp(int value);
        void ammo(int value, int max);
        void frags(int value);
        void message(std::string msg, Color c);

        void fade(Color c);
        Color fade() const { return m_Fade; }

        bool red_pulse(bool b) { m_bDirty=true;return m_RedPulse=b;}
        bool blue_pulse(bool b) { m_bDirty=true;return m_BluePulse=b;}

    private:
        
        void redraw();
        void redraw_fade();
        
        Window* m_pWindow = nullptr;
        Input* m_pInput = nullptr;
        std::shared_ptr<Canvas> m_pCanvas;
        std::shared_ptr<Canvas> m_pFadeCanvas;
        Cache<Resource, std::string>* m_pCache;
        //Pango::FontDescription m_FontDesc;
        float m_Border = 24.0f;
        Player* m_pPlayer;
        
        float m_RedPulse = 0.0f;
        bool m_RedPulsing = false;
        float m_BluePulse = 0.0f;
        bool m_BluePulsing = false;
        
        bool m_bInput = false;
        bool m_bDirty = true;
        bool m_bFadeDirty = true;

        int m_HP = 0;
        int m_Clip = 0;
        int m_Ammo = 0;
        int m_Frags = 0;
        Color m_Fade = Color(1.0f,0.0f,0.0f,0.0f);
        
        std::string m_Msg;
        Color m_MsgColor;
        Freq::Time m_MsgTime = Freq::Time(0);
};

#endif


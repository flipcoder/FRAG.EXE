#include "HUD.h"
#include "Player.h"
using namespace std;
using namespace glm;

HUD :: HUD(Player* player, Window* window, Input* input, Cache<Resource,std::string>* cache):
    m_pPlayer(player),
    m_pWindow(window),
    m_pInput(input),
    m_pCache(cache)
{
    auto sw = m_pWindow->size().x;
    auto sh = m_pWindow->size().y;

    m_pCanvas = make_shared<Canvas>(sw, sh);
    add(m_pCanvas);
    //m_pCanvas->position(vec3(0.0f, float(sh) - 72.0f, 0.0f));
    //m_pCanvas->position(vec3(0.0f, float(sh) - 72.0f, 0.0f));
    m_FontDesc = Pango::FontDescription("Audiowide Normal 56");
    m_pCanvas->layout()->set_font_description(m_FontDesc);
}

void HUD :: redraw()
{
    auto sw = m_pWindow->size().x;
    auto sh = m_pWindow->size().y;
    auto win = m_pWindow;
    
    // clear black
    //auto cairo = m_pCanvas->context();
    //cairo->set_source_rgb(0.0f, 0.0f, 0.0f);
    //cairo->paint();
    m_pCanvas->clear(Color(0.0f, 0.0f, 0.0f, 0.0f));
    
    //m_pCanvas->clear(Color(1.0f, 1.0f, 1.0f, 0.0f));
    m_pCanvas->font("Audiowide",80);
    
    Cairo::TextExtents extents;
    auto cairo = m_pCanvas->context();
    cairo->get_text_extents("+ 100%", extents);
    cairo->set_source_rgba(1.0f, 0.0f, 0.0f, 0.5f);
    m_pCanvas->rectangle(
        m_Border  - m_Border/2.0f, sh - extents.height - m_Border  - m_Border/2.0f,
        extents.width  + m_Border, extents.height  + m_Border,
        16.0f
    ); m_pCanvas->context()->fill();
    cairo->get_text_extents("100 / 100", extents);
    cairo->set_source_rgba(1.0f, 1.0f, 0.0f, 0.5f);
    m_pCanvas->rectangle(
        sw - extents.width - m_Border - m_Border/2.0f, sh - extents.height - m_Border - m_Border/2.0f,
        extents.width + m_Border, extents.height + m_Border,
        16.0f
    ); m_pCanvas->context()->fill();
    
    m_pCanvas->text("+", Color::black(), vec2(4.0f + m_Border, sh + 4.0f - m_Border), Canvas::LEFT);
    m_pCanvas->text("+", Color(1.0f,0.0f,0.0f), vec2(m_Border, sh - m_Border), Canvas::LEFT);
    m_pCanvas->text("  100%", Color::black(), vec2(4.0f + m_Border, sh + 4.0f - m_Border), Canvas::LEFT);
    m_pCanvas->text("  100%", Color::white(), vec2(m_Border,sh - m_Border), Canvas::LEFT);
    
    m_pCanvas->text("100 / 100", Color::black(), vec2(4.0f + sw - m_Border, sh + 4.0f - m_Border), Canvas::RIGHT);
    m_pCanvas->text("100 / 100", Color::white(), vec2(sw - m_Border,sh - m_Border), Canvas::RIGHT);

    m_pCanvas->font("Audiowide",40);
    cairo->get_text_extents("1000", extents);
    cairo->set_source_rgba(1.0f, 0.0f, 0.0f, 0.5f);
    m_pCanvas->rectangle(
        win->center().x - extents.width, 0.0f,
        extents.width, extents.height
    ); m_pCanvas->context()->fill();
    cairo->set_source_rgba(0.0f, 0.0f, 1.0f, 0.5f);
    m_pCanvas->rectangle(
        win->center().x, 0.0f,
        extents.width, extents.height
    ); m_pCanvas->context()->fill();

    m_pCanvas->text("0", Color::white(), vec2(win->center().x - extents.width/2.0f, extents.height), Canvas::CENTER);
    m_pCanvas->text("0", Color::white(), vec2(win->center().x + extents.width/2.0f, extents.height), Canvas::CENTER);
    
    //layout->set_text("100%");
    //layout->show_in_cairo_context(ctext);
    //m_pCanvas->dirty(false);
}

void HUD :: logic_self(Freq::Time)
{
    if(m_bDirty) {
        redraw();
        m_bDirty = false;
    }
}


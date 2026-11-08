#pragma once
#include <raylib.h>
#include <string>
namespace ui {
inline constexpr Color background{34,56,82,255}, panel{22,36,54,230}, panelLight{48,72,99,245};
inline constexpr Color line{50,80,115,255}, grid{41,67,97,255}, text{246,250,253,255}, muted{205,220,232,255};
inline constexpr Color cyan{117,207,255,255}, orange{255,184,91,255}, red{255,128,125,255};
void init();
void shutdown();
void label(const std::string& value,float x,float y,float size,Color color=text);
void box(Rectangle r,Color fill,Color border=line);
bool button(Rectangle r,const std::string& title,float scale,bool active=false);
bool hit(Rectangle r);
}

#pragma once
#include <raylib.h>
#include <string>
namespace ui {
inline constexpr Color background{25,38,52,255}, panel{19,29,41,255}, panelLight{40,55,70,255};
inline constexpr Color line{61,81,100,255}, grid{33,48,64,255}, text{255,255,255,255}, muted{195,209,221,255};
inline constexpr Color cyan{138,198,222,255}, orange{232,180,111,255}, red{255,128,125,255};
void init();
void shutdown();
// Physical-pixel size used for both layout and glyph rasterization.
int pixelSize(float size);
void label(const std::string& value,float x,float y,float size,Color color=text);
void box(Rectangle r,Color fill,Color border=line);
bool button(Rectangle r,const std::string& title,float scale,bool active=false);
bool hit(Rectangle r);
}

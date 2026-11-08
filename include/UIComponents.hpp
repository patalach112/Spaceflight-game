#pragma once
#include <raylib.h>
#include <string>

namespace ui {
inline constexpr Color background{26,34,51,255}, panel{15,23,42,255}, panelLight{36,51,74,255};
inline constexpr Color line{73,98,128,255}, grid{49,68,93,255}, text{235,242,250,255}, muted{160,181,205,255};
inline constexpr Color cyan{89,181,255,255}, orange{255,170,77,255}, red{244,116,115,255};
void init();
void shutdown();
void label(const std::string& value, float x, float y, float size, Color color = text);
void box(Rectangle r, Color fill, Color border = line);
bool button(Rectangle r, const std::string& title, float scale, bool active = false);
bool hit(Rectangle r);
}

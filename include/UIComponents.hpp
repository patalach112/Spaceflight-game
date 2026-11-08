#pragma once
#include <raylib.h>
#include <string>

namespace ui {
inline constexpr Color background{18,20,23,255}, panel{24,28,33,255}, panelLight{31,36,43,255};
inline constexpr Color line{42,48,58,255}, text{225,232,236,255}, muted{128,143,154,255};
inline constexpr Color cyan{71,213,219,255}, orange{239,145,69,255}, red{235,100,94,255};
void label(const std::string& value, float x, float y, float size, Color color = text);
void box(Rectangle r, Color fill, Color border = line);
bool button(Rectangle r, const std::string& title, float scale, bool active = false);
bool hit(Rectangle r);
}

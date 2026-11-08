#include "UIComponents.hpp"
#include <algorithm>
namespace ui {
void label(const std::string& value, float x, float y, float size, Color color) {
    DrawTextEx(GetFontDefault(), value.c_str(), {x,y}, size, std::max(1.0f, size/18.0f), color);
}
void box(Rectangle r, Color fill, Color border) { DrawRectangleRec(r, fill); DrawRectangleLinesEx(r, 1, border); }
bool hit(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(), r); }
bool button(Rectangle r, const std::string& title, float scale, bool active) {
    bool hover = hit(r);
    box(r, active ? Color{34,58,62,255} : hover ? Color{40,47,55,255} : panelLight, active ? cyan : hover ? muted : line);
    if (active) DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), 3, static_cast<int>(r.height), cyan);
    label(title, r.x + 13*scale, r.y + (r.height-15*scale)/2, 15*scale, active ? cyan : text);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
}

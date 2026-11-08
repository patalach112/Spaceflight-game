#include "UIComponents.hpp"
#include <algorithm>
namespace ui {
namespace { Font font{}; bool ownsFont = false; }
void init() {
    std::string path = std::string(GetApplicationDirectory()) + "assets/DejaVuSans.ttf";
    if (FileExists(path.c_str())) {
        font = LoadFontEx(path.c_str(), 64, nullptr, 0);
        ownsFont = font.texture.id != 0 && font.texture.id != GetFontDefault().texture.id;
        if (ownsFont) SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    }
    if (!ownsFont) { TraceLog(LOG_WARNING, "UI font not found; using Raylib default"); font = GetFontDefault(); }
}
void shutdown() { if (ownsFont) UnloadFont(font); ownsFont = false; font = {}; }
void label(const std::string& value, float x, float y, float size, Color color) {
    DrawTextEx(font, value.c_str(), {x,y}, size, std::max(0.2f, size/50.0f), color);
}
void box(Rectangle r, Color fill, Color border) { DrawRectangleRec(r, fill); DrawRectangleLinesEx(r, 1, border); }
bool hit(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(), r); }
bool button(Rectangle r, const std::string& title, float scale, bool active) {
    bool hover = hit(r);
    box(r, active ? Color{35,86,135,255} : hover ? Color{53,76,105,255} : panelLight, active ? cyan : hover ? muted : line);
    if (active) DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), 3, static_cast<int>(r.height), orange);
    label(title, r.x + 13*scale, r.y + (r.height-15*scale)/2, 15*scale, text);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
}

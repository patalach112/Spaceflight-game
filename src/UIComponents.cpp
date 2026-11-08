#include "UIComponents.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace ui {
namespace {
constexpr int minimumFontSize = 12;
constexpr int maximumFontSize = 96;
struct FontSlot { Font font{}; bool attempted = false; bool owned = false; };
std::array<FontSlot, maximumFontSize + 1> fonts{};
std::string fontPath;
bool warnedAboutFont = false;

// Rasterize each requested integer size once. Rendering at that exact base size
// keeps glyph advances and atlas sampling aligned to physical pixels.
Font fontAt(int size) {
    auto& slot = fonts[static_cast<std::size_t>(size)];
    if (!slot.attempted) {
        slot.attempted = true;
        if (FileExists(fontPath.c_str())) {
            slot.font = LoadFontEx(fontPath.c_str(), size, nullptr, 0);
            slot.owned = slot.font.texture.id != 0 && slot.font.texture.id != GetFontDefault().texture.id;
        }
        if (slot.owned) SetTextureFilter(slot.font.texture, TEXTURE_FILTER_POINT);
        else {
            // A failed GPU upload can still leave allocated glyph data behind.
            if (slot.font.texture.id != GetFontDefault().texture.id &&
                (slot.font.glyphs != nullptr || slot.font.recs != nullptr)) UnloadFont(slot.font);
            slot.font = GetFontDefault();
            if (!warnedAboutFont) {
                TraceLog(LOG_WARNING, "UI font unavailable: %s; using Raylib default", fontPath.c_str());
                warnedAboutFont = true;
            }
        }
    }
    return slot.font;
}
}
void init() {
    fontPath = std::string(GetApplicationDirectory()) + "assets/DejaVuSans.ttf";
    warnedAboutFont = false;
}
void shutdown() {
    for (auto& slot : fonts) {
        if (slot.owned) UnloadFont(slot.font);
        slot = {};
    }
    fontPath.clear();
}
int pixelSize(float size) { return std::clamp(static_cast<int>(std::round(size)), minimumFontSize, maximumFontSize); }
void label(const std::string& value, float x, float y, float size, Color color) {
    const int pixels = pixelSize(size);
    color.a = 255;
    DrawTextEx(fontAt(pixels), value.c_str(), {std::floor(x), std::floor(y)}, static_cast<float>(pixels), 0.0f, color);
}
void box(Rectangle r, Color fill, Color border) { DrawRectangleRec(r, fill); DrawRectangleLinesEx(r, 1, border); }
bool hit(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(), r); }
bool button(Rectangle r, const std::string& title, float scale, bool active) {
    bool hover = hit(r);
    box(r, active ? Color{49,101,145,250} : hover ? Color{70,101,131,245} : panelLight, active ? cyan : line);
    if (active) DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), 3, static_cast<int>(r.height), orange);
    label(title, r.x + 10*scale, r.y + (r.height - std::max(12.0f, 14*scale))/2, 14*scale, text);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
}

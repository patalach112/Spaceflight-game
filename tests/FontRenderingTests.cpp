// Standalone headless regression test with Raylib mocks; no window is required.
// g++ -std=c++20 -Iinclude -I<raylib-source>/src src/UIComponents.cpp tests/FontRenderingTests.cpp -o font-tests
#include "UIComponents.hpp"
#include <cassert>
#include <iostream>

namespace {
int loads = 0, filters = 0, unloads = 0, draws = 0, lastSize = 0;
Vector2 position{};
Color tint{};
bool fontExists = true;
}
extern "C" {
const char* GetApplicationDirectory(void) { return "/test/"; }
bool FileExists(const char*) { return fontExists; }
Font GetFontDefault(void) { Font font{}; font.texture.id = 1; return font; }
Font LoadFontEx(const char*, int size, int*, int) {
    ++loads;
    Font font{}; font.baseSize = size; font.texture.id = static_cast<unsigned>(size + 100);
    return font;
}
void SetTextureFilter(Texture2D, int filter) { assert(filter == TEXTURE_FILTER_POINT); ++filters; }
void UnloadFont(Font font) { assert(font.texture.id != 1); ++unloads; }
void TraceLog(int, const char*, ...) {}
void DrawTextEx(Font font, const char*, Vector2 p, float size, float spacing, Color color) {
    ++draws;
    assert(font.texture.id == 1 || font.baseSize == static_cast<int>(size));
    assert(spacing == 0);
    position = p; tint = color; lastSize = static_cast<int>(size);
}
void DrawRectangleRec(Rectangle, Color) {}
void DrawRectangleLinesEx(Rectangle, float, Color) {}
bool CheckCollisionPointRec(Vector2, Rectangle) { return false; }
Vector2 GetMousePosition(void) { return {}; }
void DrawRectangle(int, int, int, int, Color) {}
bool IsMouseButtonPressed(int) { return false; }
}
int main() {
    ui::init();
    assert(ui::pixelSize(8)==12 && ui::pixelSize(13.4f)==13 && ui::pixelSize(13.6f)==14 && ui::pixelSize(100)==96);
    ui::label("Test", 10.9f, -2.1f, 13.4f, {220,230,240,30});
    assert(loads == 1 && filters == 1 && position.x == 10 && position.y == -3);
    assert(tint.a == 255 && lastSize == 13);
    ui::label("Again", 0, 0, 13, {255,255,255,255});
    assert(loads == 1); // No rerasterization for an already cached size.
    ui::label("Small", 0, 0, 8, {255,255,255,255});
    assert(lastSize == 12 && loads == 2);
    ui::shutdown();
    assert(unloads == 2 && draws == 3);

    fontExists = false;
    ui::init();
    ui::label("Fallback", 0, 0, 14, {255,255,255,255});
    ui::shutdown();
    assert(loads == 2 && unloads == 2); // The default font is not owned.
    std::cout << "Font pixel alignment, caching, opacity and point filtering passed\n";
}

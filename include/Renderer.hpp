#pragma once
#include "VABSystem.hpp"
#include <raylib.h>
struct Viewport { float originX, originY, zoom; };
class Renderer {
public:
    static Vector2 toScreen(double gx, double gy, const Viewport& view);
    static Vector2 toGrid(Vector2 point, const Viewport& view);
    static void workspace(const Viewport& view, Rectangle area);
    static void part(const PartDef& def, double x, double y, int rotation, const Viewport& view, Color tint, bool selected = false, bool flipped = false, bool deployed = false);
    static void tilePart(const PartDef& def, Rectangle area, bool deployed = false);
};

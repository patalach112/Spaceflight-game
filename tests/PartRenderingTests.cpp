// Standalone renderer regression test using Raylib mocks, without a window.
// g++ -std=c++20 -Iinclude -I<raylib-source>/src src/PartCatalog.cpp src/Renderer.cpp tests/PartRenderingTests.cpp -o part-tests
#include "Renderer.hpp"
#include <rlgl.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
struct Vertex { Vector2 position; Color color; };
struct Line { Vector2 a, b; float width; };
std::vector<Vertex> vertices;
std::vector<Line> lines;
std::vector<Rectangle> gradientBands;
Color currentColor{};
Vector2 translation{};
bool near(float a, float b) { return std::abs(a-b)<0.001f; }
bool equal(Vector2 a, Vector2 b) { return near(a.x,b.x) && near(a.y,b.y); }
bool equal(Color a, Color b) { return a.r==b.r && a.g==b.g && a.b==b.b && a.a==b.a; }
bool hasLine(Vector2 a, Vector2 b) {
    for (auto l : lines) if (equal(l.a,a) && equal(l.b,b) && near(l.width,1)) return true;
    return false;
}
}
extern "C" {
Texture2D GetShapesTexture(void) { return {1,256,256,1,7}; }
Rectangle GetShapesTextureRectangle(void) { return {0,0,1,1}; }
void rlSetTexture(unsigned int) {}
void rlBegin(int mode) { assert(mode == RL_TRIANGLES); vertices.clear(); }
void rlEnd(void) {}
void rlTexCoord2f(float, float) {}
void rlColor4ub(unsigned char r,unsigned char g,unsigned char b,unsigned char a) { currentColor={r,g,b,a}; }
void rlVertex2f(float x,float y) { vertices.push_back({{x,y},currentColor}); }
void DrawLineEx(Vector2 a,Vector2 b,float width,Color) {
    lines.push_back({{a.x+translation.x,a.y+translation.y},{b.x+translation.x,b.y+translation.y},width});
}
void DrawLineV(Vector2,Vector2,Color) {}
void DrawRectangleRec(Rectangle,Color) {}
void DrawRectangleGradientEx(Rectangle rect,Color,Color,Color,Color) { gradientBands.push_back(rect); }
void DrawCircleV(Vector2,float,Color) {}
void DrawCircleLinesV(Vector2,float,Color) {}
void DrawRectangleLinesEx(Rectangle,float,Color) {}
void BeginScissorMode(int,int,int,int) {}
void EndScissorMode(void) {}
void BeginMode2D(Camera2D camera) { assert(camera.rotation == 0); translation=camera.offset; }
void EndMode2D(void) { translation={}; }
}
int main() {
    Viewport view{0,0,1};
    Color white{255,255,255,255};
    // The pod's bottom face and tank's top face share the exact world-space edge.
    Renderer::part(partById(0),-2,-13,0,view,white);
    assert(vertices.size()==32*6);
    for (std::size_t i=0;i<31;++i) {
        const auto& rightBottom=vertices[i*6+2];
        const auto& rightTop=vertices[i*6+5];
        const auto& nextBottom=vertices[(i+1)*6+1];
        const auto& nextTop=vertices[(i+1)*6];
        assert(equal(rightBottom.position,nextBottom.position));
        assert(equal(rightTop.position,nextTop.position));
        assert(equal(rightBottom.color,nextBottom.color));
        assert(equal(rightTop.color,nextTop.color));
    }
    assert(hasLine({-40,-180},{40,-180}));
    lines.clear(); gradientBands.clear();
    Renderer::part(partById(1),-2,-9,0,view,white);
    assert(hasLine({-40,-180},{40,-180}));
    assert(hasLine({-40,-60},{40,-60}));
    assert(gradientBands.size()==3);
    assert(near(gradientBands.front().x,0));
    assert(near(gradientBands.back().x+gradientBands.back().width,80));
    lines.clear();
    Renderer::part(partById(2),-2,-3,0,view,white);
    assert(hasLine({-40,-60},{40,-60})); // Engine flange meets the tank bottom.
    assert(vertices.size()==32*6);
    for (const auto& vertex : vertices) {
        assert(vertex.position.x>=0 && vertex.position.x<=80);
        assert(vertex.position.y>=0 && vertex.position.y<=60);
    }
    std::cout << "Shared gradient vertices and flush attachment faces passed\n";
}

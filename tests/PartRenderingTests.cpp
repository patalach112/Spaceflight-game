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
int pushes=0, pops=0, scissors=0, cullOff=0, cullOn=0, flushes=0;
std::vector<Color> bandColors;
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
void rlDrawRenderBatchActive(void) { ++flushes; }
void rlDisableBackfaceCulling(void) { ++cullOff; }
void rlEnableBackfaceCulling(void) { ++cullOn; }
void rlPushMatrix(void) { ++pushes; }
void rlPopMatrix(void) { ++pops; }
void rlTranslatef(float,float,float) {}
void rlScalef(float,float,float) {}
void rlBegin(int mode) { assert(mode == RL_TRIANGLES); vertices.clear(); }
void rlEnd(void) {}
void rlTexCoord2f(float, float) {}
void rlColor4ub(unsigned char r,unsigned char g,unsigned char b,unsigned char a) { currentColor={r,g,b,a}; }
void rlVertex2f(float x,float y) { vertices.push_back({{x,y},currentColor}); }
void DrawLineEx(Vector2 a,Vector2 b,float width,Color) {
    lines.push_back({{a.x+translation.x,a.y+translation.y},{b.x+translation.x,b.y+translation.y},width});
}
void DrawLineV(Vector2,Vector2,Color) {}
void DrawTriangle(Vector2,Vector2,Vector2,Color) {}
void DrawRectangleRec(Rectangle,Color) {}
void DrawRectangleGradientEx(Rectangle rect,Color a,Color,Color,Color d) { gradientBands.push_back(rect); bandColors.push_back(a); bandColors.push_back(d); }
void DrawCircleV(Vector2,float,Color) {}
void DrawCircleLinesV(Vector2,float,Color) {}
void DrawRectangleLinesEx(Rectangle,float,Color) {}
void BeginScissorMode(int,int,int,int) { ++scissors; }
void EndScissorMode(void) {}
void BeginMode2D(Camera2D camera) { assert(std::fmod(camera.rotation,90.0f) == 0); translation=camera.offset; }
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
    bool steelEdge=false, steelBody=false;
    for(auto c:bandColors) { steelEdge|=c.r==96 && c.g==115 && c.b==138; steelBody|=c.r==138 && c.g==155 && c.b==176; }
    assert(steelEdge && steelBody);
    bandColors.clear();
    Renderer::part(partById(1),0,0,0,view,Color{255,128,125,180});
    assert(!bandColors.empty() && bandColors.front().r==96 && bandColors.front().g==57 && bandColors.front().a==180);
    for(const auto& d:partCatalog()) {
        Renderer::tilePart(d,{0,0,53,58});
        for(int rotation=0; rotation<4; ++rotation) Renderer::part(d,4,3,rotation,view,white,false,true);
    }
    assert(scissors==0); // tile rendering must preserve its caller's list scissor
    assert(pushes==pops && pushes==static_cast<int>(partCatalog().size())*4);
    assert(cullOff==pushes && cullOn==pushes && flushes==2*pushes);
    lines.clear(); Renderer::workspace(view,{-100,-100,200,200});
    for(const auto& line:lines) assert(!(near(line.a.y,line.b.y) && line.width>1)); // no ground barrier
    std::cout << "Shared gradients, flat clipped tiles and reflected part rendering passed\n";
}

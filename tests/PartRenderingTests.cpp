// Standalone renderer regression test using Raylib mocks, without a window.
// g++ -std=c++20 -Iinclude -I<raylib-source>/src src/PartCatalog.cpp src/Renderer.cpp tests/PartRenderingTests.cpp -o part-tests
#include "Renderer.hpp"
#include <rlgl.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <set>
#include <string>
#include <vector>

namespace {
struct Vertex { Vector2 position; Color color; };
struct Line { Vector2 a, b; float width; Color color; };
struct Triangle { Vector2 a,b,c; Color color; };
std::vector<Vertex> vertices;
std::vector<Line> lines;
std::vector<Triangle> triangles;
int circles=0;
std::vector<Rectangle> gradientBands;
Color currentColor{};
int pushes=0, pops=0, scissors=0, cullOff=0, cullOn=0, flushes=0;
std::vector<Color> bandColors;
Vector2 translation{};
unsigned int activeTexture=0;
bool recording=false;
float normX=0,normY=0,normW=1,normH=1;
std::ostringstream signature;
void xy(Vector2 p) {
    float x=(p.x-normX)/normW,y=(p.y-normY)/normH;
    assert(x>=-.001f && x<=1.001f && y>=-.001f && y<=1.001f);
    signature << x << ',' << y << ';';
}
void box(Rectangle r) { xy({r.x,r.y}); signature << r.width/normW << ',' << r.height/normH << ';'; }
bool near(float a, float b) { return std::abs(a-b)<0.001f; }
bool equal(Vector2 a, Vector2 b) { return near(a.x,b.x) && near(a.y,b.y); }
bool equal(Color a, Color b) { return a.r==b.r && a.g==b.g && a.b==b.b && a.a==b.a; }
bool hasLine(Vector2 a, Vector2 b) {
    for (auto l : lines) if (((equal(l.a,a) && equal(l.b,b)) || (equal(l.a,b) && equal(l.b,a))) && near(l.width,1)) return true;
    return false;
}
}
extern "C" {
Texture2D GetShapesTexture(void) { return {1,256,256,1,7}; }
Rectangle GetShapesTextureRectangle(void) { return {0,0,1,1}; }
unsigned int rlGetTextureIdDefault(void) { return 77; }
void rlSetTexture(unsigned int id) { activeTexture=id; }
void rlDrawRenderBatchActive(void) { ++flushes; }
void rlDisableBackfaceCulling(void) { ++cullOff; }
void rlEnableBackfaceCulling(void) { ++cullOn; }
void rlPushMatrix(void) { ++pushes; }
void rlPopMatrix(void) { ++pops; }
void rlTranslatef(float,float,float) {}
void rlScalef(float,float,float) {}
void rlBegin(int mode) { assert(mode == RL_QUADS && activeTexture == 77); vertices.clear(); if(recording) signature << "mesh;"; }
void rlEnd(void) {}
void rlTexCoord2f(float, float) {}
void rlColor4ub(unsigned char r,unsigned char g,unsigned char b,unsigned char a) { currentColor={r,g,b,a}; }
void rlVertex2f(float x,float y) { vertices.push_back({{x,y},currentColor}); if(recording) xy({x,y}); }
void DrawLineEx(Vector2 a,Vector2 b,float width,Color color) {
    lines.push_back({{a.x+translation.x,a.y+translation.y},{b.x+translation.x,b.y+translation.y},width,color});
    if(recording) { signature << "line;"; xy(a); xy(b); }
}
void DrawLineV(Vector2,Vector2,Color) {}
void DrawTriangle(Vector2 a,Vector2 b,Vector2 c,Color color) { triangles.push_back({a,b,c,color}); assert((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x)<=0.001f); if(recording) { signature << "triangle;"; xy(a); xy(b); xy(c); } }
void DrawRectangleRec(Rectangle r,Color) { if(recording) { signature << "rect;"; box(r); } }
void DrawRectangleGradientEx(Rectangle rect,Color a,Color,Color,Color d) { gradientBands.push_back(rect); bandColors.push_back(a); bandColors.push_back(d); if(recording) { signature << "gradient;"; box(rect); } }
void DrawCircleV(Vector2 p,float r,Color) { ++circles; if(recording) { signature << "circle;"; xy(p); signature << r/std::min(normW,normH) << ';'; } }
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
    assert(vertices.size()==32*4);
    for (std::size_t i=0;i<31;++i) {
        const auto& rightBottom=vertices[i*4+2];
        const auto& rightTop=vertices[i*4+3];
        const auto& nextBottom=vertices[(i+1)*4+1];
        const auto& nextTop=vertices[(i+1)*4];
        assert(equal(rightBottom.position,nextBottom.position));
        assert(equal(rightTop.position,nextTop.position));
        assert(equal(rightBottom.color,nextBottom.color));
        assert(equal(rightTop.color,nextTop.color));
    }
    assert(hasLine({-40,-180},{40,-180}));
    assert(circles==0 && triangles.empty()); // no capsule porthole or floating window
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
    assert(vertices.size()==32*5*4);
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
    for(int id:{8,9,2,10,11}) {
        const auto& d=partById(id); float h=d.height*20.0f;
        triangles.clear(); lines.clear();
        Renderer::part(d,0,0,0,view,white);
        assert(vertices.size()==32*5*4 && triangles.size()>=7*32);
        for(int slice=0;slice<31;++slice) for(int row=0;row<5;++row) {
            const auto base=(slice*5+row)*4, neighbor=((slice+1)*5+row)*4;
            assert(equal(vertices[base+2].position,vertices[neighbor+1].position));
            assert(equal(vertices[base+3].position,vertices[neighbor].position));
            assert(equal(vertices[base+2].color,vertices[neighbor+1].color));
            assert(equal(vertices[base+3].color,vertices[neighbor].color));
        }
        // Each bell has a dark throat and an inset dark exhaust opening.
        bool throat=false,mouth=false,metalRim=false;
        for(const auto& t:triangles) {
            float centerY=(t.a.y+t.b.y+t.c.y)/3;
            bool dark=t.color.r<=30 && t.color.g<=44 && t.color.b<=58;
            throat |= dark && centerY<h*.42f;
            mouth |= dark && centerY>h*.90f;
            metalRim |= t.color.r==135 && t.color.g==147 && centerY>h*.90f;
        }
        assert(throat && mouth && metalRim);
        // Vertical shading changes across the bell and shares vertices/colors
        // at the join between adjacent bands.
        const auto& lower=vertices[(16*5+1)*4+1];
        const auto& next=vertices[(16*5+2)*4];
        assert(equal(lower.position,next.position) && equal(lower.color,next.color));
        const auto& end=vertices[(16*5+2)*4+1];
        assert(!equal(lower.color,end.color));
        for(const auto& line:lines) if(equal(line.color,Color{30,44,58,255})) assert(near(line.width,1));
        triangles.clear();
        Renderer::part(d,0,0,0,view,Color{255,128,125,180});
        for(const auto& vertex:vertices) assert(vertex.color.a==180);
        for(const auto& t:triangles) assert(t.color.a==180);
    }
    std::set<std::string> designs;
    for(const auto& d:partCatalog()) {
        // Compare normalized geometry, so varying dimensions alone do not make
        // otherwise identical silhouettes appear to be unique.
        float unit=std::min(53.0f/(d.width+1.0f),58.0f/(d.height+1.0f));
        normW=d.width*unit; normH=d.height*unit; normX=(53-normW)/2; normY=(58-normH)/2;
        signature.str(""); signature.clear(); recording=true;
        Renderer::tilePart(d,{0,0,53,58});
        recording=false;
        std::string art=signature.str();
        if(!designs.insert(art).second) { std::cerr << "duplicate art for " << d.name << '\n'; return 1; }
        if(d.shape==PartShape::Capsule) assert(art.find("circle;")==std::string::npos && art.find("triangle;")==std::string::npos && art.find("rect;")==std::string::npos);
        if(d.shape==PartShape::Truss) assert(art.find("gradient;")==std::string::npos && art.find("rect;")==std::string::npos && art.find("mesh;")==std::string::npos);
        if(d.shape==PartShape::LandingLeg) assert(art.find("triangle;")!=std::string::npos && art.find("circle;")!=std::string::npos && art.find("mesh;")==std::string::npos);
        if(d.shape==PartShape::Parachute || d.shape==PartShape::HeatShield || d.shape==PartShape::Engine) assert(art.find("mesh;")!=std::string::npos);
        lines.clear(); Renderer::part(d,0,0,0,view,white);
        bool outline=false;
        for(const auto& line:lines) if(equal(line.color,Color{30,44,58,255})) { outline=true; assert(near(line.width,1)); }
        assert(outline); // dark slate outlines remain a crisp one pixel
        for(int rotation=0; rotation<4; ++rotation) Renderer::part(d,4,3,rotation,view,white,false,true);
    }
    assert(designs.size()==22);
    assert(scissors==0); // tile rendering must preserve its caller's list scissor
    assert(pushes==pops && pushes==static_cast<int>(partCatalog().size())*4);
    assert(cullOff==pushes && cullOn==pushes && flushes==2*pushes);
    lines.clear(); Renderer::workspace(view,{-100,-100,200,200});
    for(const auto& line:lines) assert(!(near(line.a.y,line.b.y) && line.width>1)); // no ground barrier
    std::cout << "22 unique vector designs, white-pixel meshes, attachment faces and reflection passed\n";
}

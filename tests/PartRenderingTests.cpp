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
std::vector<Line> lines, gridLines;
Color lastRectangleColor{};
Rectangle lastRectangle{};
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
std::vector<float> normalizedGeometry;
void xy(Vector2 p) {
    float x=(p.x-normX)/normW,y=(p.y-normY)/normH;
    assert(x>=-.001f && x<=1.001f && y>=-.001f && y<=1.001f);
    signature << x << ',' << y << ';';
    normalizedGeometry.push_back(x); normalizedGeometry.push_back(y);
}
void box(Rectangle r) {
    xy({r.x,r.y}); signature << r.width/normW << ',' << r.height/normH << ';';
    normalizedGeometry.push_back(r.width/normW); normalizedGeometry.push_back(r.height/normH);
}
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
void DrawLineV(Vector2 a,Vector2 b,Color color) { gridLines.push_back({a,b,1,color}); }
void DrawTriangle(Vector2 a,Vector2 b,Vector2 c,Color color) { triangles.push_back({a,b,c,color}); assert((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x)<=0.001f); if(recording) { signature << "triangle;"; xy(a); xy(b); xy(c); } }
void DrawRectangleRec(Rectangle r,Color color) { lastRectangle=r; lastRectangleColor=color; if(recording) { signature << "rect;"; box(r); } }
void DrawRectangleGradientEx(Rectangle rect,Color a,Color,Color,Color d) { gradientBands.push_back(rect); bandColors.push_back(a); bandColors.push_back(d); if(recording) { signature << "gradient;"; box(rect); } }
void DrawCircleV(Vector2 p,float r,Color) { ++circles; if(recording) { signature << "circle;"; xy(p); signature << r/std::min(normW,normH) << ';'; normalizedGeometry.push_back(r/std::min(normW,normH)); } }
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
    assert(hasLine({-20,-260},{-16,-260})); // flat crown for the top attachment
    assert(circles==0 && triangles.empty()); // no capsule porthole or floating window
    lines.clear(); gradientBands.clear();
    Renderer::part(partById(1),-2,-9,0,view,white);
    assert(hasLine({-40,-180},{40,-180}));
    assert(hasLine({-40,-60},{40,-60}));
    assert(gradientBands.size()==5);
    assert(bandColors.size()==10);
    for(std::size_t i=0;i+1<5;++i) assert(equal(bandColors[2*i+1],bandColors[2*i+2]));
    assert(near(gradientBands.front().x,0));
    assert(near(gradientBands.back().x+gradientBands.back().width,80));
    lines.clear();
    Renderer::part(partById(2),-2,-3,0,view,white);
    assert(hasLine({-40,-60},{40,-60})); // Engine flange meets the tank bottom.
    assert(vertices.size()==48*5*4);
    for (const auto& vertex : vertices) {
        assert(vertex.position.x>=0 && vertex.position.x<=80);
        assert(vertex.position.y>=0 && vertex.position.y<=partById(2).height*20);
    }
    bool steelEdge=false, steelBody=false;
    for(auto c:bandColors) { steelEdge|=c.r==104 && c.g==109 && c.b==113; steelBody|=c.r==177 && c.g==182 && c.b==183; }
    assert(steelEdge && steelBody);
    bandColors.clear();
    Renderer::part(partById(1),0,0,0,view,Color{255,128,125,180});
    assert(!bandColors.empty() && bandColors.front().r==104 && bandColors.front().g==54 && bandColors.front().a==180);
    std::set<std::string> bellContours;
    for(int id:{8,9,2,10,11}) {
        const auto& d=partById(id); float h=d.height*20.0f, w=d.width*20.0f;
        triangles.clear(); lines.clear(); gradientBands.clear(); circles=0;
        Renderer::part(d,0,0,0,view,white);
        constexpr int rows=48,slices=5;
        struct Profile { float top,bottom,neck,c1,c2,lip; };
        Profile p{};
        switch(id) {
            case 8: p={.405f,.955f,.085f,.105f,.16f,.275f}; break;
            case 9: p={.365f,.955f,.14f,.23f,.37f,.445f}; break;
            case 2: p={.30f,.955f,.11f,.20f,.31f,.405f}; break;
            case 10:p={.38f,.955f,.095f,.14f,.29f,.395f}; break;
            case 11:p={.34f,.955f,.095f,.15f,.24f,.32f}; break;
        }
        assert((p.bottom-p.top)*h>=2*p.lip*w*.98f);
        assert(vertices.size()==static_cast<std::size_t>(rows*slices*4) && triangles.size()>=6*32);
        assert(gradientBands.size()>=10); // mounting hardware uses multi-stop metal
        if(id==9) {
            // Both auxiliary outlets and the broad central bell must be present.
            assert(hasLine({.19f*w,.405f*h},{.065f*w,.405f*h}));
            assert(hasLine({.935f*w,.405f*h},{.81f*w,.405f*h}));
            assert(circles==8); // four two-part flange fasteners
        }
        if(id==8) {
            assert(hasLine({.19f*w,.105f*h},{.19f*w,.31f*h}));
            assert(hasLine({.81f*w,.105f*h},{.81f*w,.31f*h}));
        }
        if(id==11) assert(circles==0); // the micro-engine stays deliberately minimal
        auto radius=[&](float t) { float u=1-t; return u*u*u*p.neck+3*u*u*t*p.c1+3*u*t*t*p.c2+t*t*t*p.lip; };
        std::ostringstream contour;
        for(int row=0;row<rows;++row) {
            int rowStart=row*slices*4;
            float t=static_cast<float>(row)/rows;
            assert(near(vertices[rowStart].position.x/w,.5f-radius(t)));
            assert(near(vertices[rowStart].position.y/h,p.top+(p.bottom-p.top)*t));
            for(int i:{0,1,(slices-1)*4+2,(slices-1)*4+3})
                contour << vertices[rowStart+i].position.x/w << ',' << vertices[rowStart+i].position.y/h << ';';
            for(int slice=0;slice<slices-1;++slice) {
                int base=rowStart+slice*4, neighbor=base+4;
                assert(equal(vertices[base+2].position,vertices[neighbor+1].position));
                assert(equal(vertices[base+3].position,vertices[neighbor].position));
                assert(equal(vertices[base+2].color,vertices[neighbor+1].color));
                assert(equal(vertices[base+3].color,vertices[neighbor].color));
            }
            if(row+1<rows) for(int slice=0;slice<slices;++slice) {
                int base=rowStart+slice*4, neighbor=(row+1)*slices*4+slice*4;
                assert(equal(vertices[base+1].position,vertices[neighbor].position));
                assert(equal(vertices[base+2].position,vertices[neighbor+3].position));
                assert(equal(vertices[base+1].color,vertices[neighbor].color));
                assert(equal(vertices[base+2].color,vertices[neighbor+3].color));
            }
        }
        assert(bellContours.insert(contour.str()).second);
        // Adjacent tessellation stations form a smooth authored curve, not a
        // handful of flat-sided cone segments. Check the steel-blue palette.
        for(int row=1;row<rows-1;++row) {
            float before=radius(static_cast<float>(row-1)/rows);
            float current=radius(static_cast<float>(row)/rows);
            float after=radius(static_cast<float>(row+1)/rows);
            assert(std::abs(after-2*current+before)<.001f);
        }
        bool blueGrey=false, highlight=false;
        for(const auto& vertex:vertices) {
            auto color=vertex.color;
            blueGrey |= color.b>color.g && color.g>color.r && color.r>=48 && color.r<=75;
            highlight |= color.b>color.g && color.g>color.r && color.r>=105;
        }
        assert(blueGrey && highlight);
        bool throat=false,mouth=false,metalRim=false;
        for(const auto& t:triangles) {
            float centerY=(t.a.y+t.b.y+t.c.y)/3;
            bool dark=t.color.r<=30 && t.color.g<=44 && t.color.b<=58;
            throat |= dark && centerY<h*.42f;
            mouth |= dark && centerY>h*.90f;
            metalRim |= t.color.r==135 && t.color.g==147 && centerY>h*.90f;
        }
        assert(throat && mouth && metalRim);
        for(const auto& line:lines) if(equal(line.color,Color{26,38,52,255})) assert(near(line.width,1));
        triangles.clear(); lines.clear(); bandColors.clear();
        Renderer::part(d,0,0,0,view,Color{255,128,125,180});
        for(const auto& vertex:vertices) assert(vertex.color.a==180);
        for(const auto& t:triangles) assert(t.color.a==180);
        for(const auto& l:lines) assert(l.color.a==180);
        for(const auto& color:bandColors) assert(color.a==180);
    }
    assert(bellContours.size()==5);
    // Critical recognisable hardware and exact attachment-facing features.
    lines.clear(); Renderer::part(partById(14),0,0,0,view,white);
    assert(hasLine({18.4f,76.8f},{40.0f,76.8f})); // landing-leg footpad
    lines.clear(); Renderer::part(partById(21),0,0,0,view,white);
    assert(hasLine({10.8f,0},{29.2f,0}) && hasLine({0,20},{40,20})); // port capture rim and base
    lines.clear(); Renderer::part(partById(20),0,0,0,view,white);
    bool shieldTouchesBottom=false;
    for(const auto& line:lines) shieldTouchesBottom |= equal(line.a,{40,20}) || equal(line.b,{40,20});
    assert(shieldTouchesBottom); // the curved heat shield reaches its bottom node
    gradientBands.clear(); Renderer::part(partById(13),0,0,0,view,white);
    assert(gradientBands.size()==10); // two separate side-separator housings
    assert(gradientBands[4].x+gradientBands[4].width < gradientBands[5].x);
    std::set<std::string> designs;
    std::set<unsigned> materialEdges;
    for(const auto& d:partCatalog()) {
        // Compare normalized geometry, so varying dimensions alone do not make
        // otherwise identical silhouettes appear to be unique.
        float unit=std::min(53.0f/(d.width+1.0f),58.0f/(d.height+1.0f));
        normW=d.width*unit; normH=d.height*unit; normX=(53-normW)/2; normY=(58-normH)/2;
        signature.str(""); signature.clear(); vertices.clear(); bandColors.clear(); lines.clear(); recording=true;
        Renderer::tilePart(d,{0,0,53,58});
        recording=false;
        std::string art=signature.str();
        // Preserve the distinct original materials. New families are also
        // checked for unique normalized geometry, including their detailing.
        if(d.id<22) {
            Color edge{};
            if(d.shape==PartShape::Truss) { assert(!lines.empty()); edge=lines.front().color; }
            else if(d.shape==PartShape::Capsule || d.shape==PartShape::NoseCone || d.shape==PartShape::SideNose || d.shape==PartShape::Parachute || d.shape==PartShape::HeatShield) {
                assert(!vertices.empty()); edge=vertices.front().color;
            } else { assert(!bandColors.empty()); edge=bandColors.front(); }
            unsigned key=(static_cast<unsigned>(edge.r)<<16)|(static_cast<unsigned>(edge.g)<<8)|edge.b;
            assert(materialEdges.insert(key).second);
        }
        if(!designs.insert(art).second) { std::cerr << "duplicate art for " << d.name << '\n'; return 1; }
        if(d.shape==PartShape::Capsule) assert(art.find("circle;")==std::string::npos && art.find("triangle;")==std::string::npos && art.find("rect;")==std::string::npos);
        if(d.shape==PartShape::Truss) assert(art.find("gradient;")==std::string::npos && art.find("rect;")==std::string::npos && art.find("mesh;")==std::string::npos);
        if(d.shape==PartShape::LandingLeg) assert(art.find("triangle;")!=std::string::npos && art.find("circle;")!=std::string::npos && art.find("mesh;")==std::string::npos);
        if(d.shape==PartShape::Parachute || d.shape==PartShape::HeatShield || d.shape==PartShape::Engine) assert(art.find("mesh;")!=std::string::npos);
        lines.clear(); Renderer::part(d,0,0,0,view,white);
        bool outline=false;
        for(const auto& line:lines) if(equal(line.color,Color{26,38,52,255})) { outline=true; assert(near(line.width,1)); }
        assert(outline); // dark slate outlines remain a crisp one pixel
        for(int rotation=0; rotation<4; ++rotation) Renderer::part(d,4,3,rotation,view,white,false,true);
    }
    assert(designs.size()==partCatalog().size() && materialEdges.size()==22);
    assert(scissors==0); // tile rendering must preserve its caller's list scissor
    assert(pushes==pops && pushes==static_cast<int>(partCatalog().size())*4);
    assert(cullOff==pushes && cullOn==pushes && flushes==2*pushes);

    // Geometry must be identical after removing one uniform scale and offset,
    // regardless of whether it is drawn in the VAB or in a wide/tall tile.
    // This compares all emitted coordinates, not merely the outer rectangle.
    for(const auto& d:partCatalog()) {
        normX=normY=0; normW=d.width*20.0f; normH=d.height*20.0f;
        normalizedGeometry.clear(); signature.str(""); signature.clear(); recording=true;
        Renderer::part(d,0,0,0,view,white);
        recording=false;
        const auto reference=normalizedGeometry;
        assert(!reference.empty());
        auto matches=[&] {
            assert(reference.size()==normalizedGeometry.size());
            for(std::size_t i=0;i<reference.size();++i)
                assert(std::abs(reference[i]-normalizedGeometry[i])<0.00002f);
        };
        for(float zoom:{.5f,1.0f,3.5f}) {
            normX=normY=0; normW=d.width*20*zoom; normH=d.height*20*zoom;
            normalizedGeometry.clear(); recording=true;
            Renderer::part(d,0,0,0,Viewport{0,0,zoom},white);
            recording=false;
            matches();
        }
        for(Rectangle area: {Rectangle{17,31,800,1000},Rectangle{17,31,1100,440},Rectangle{17,31,440,1100}}) {
            float unit=std::min(area.width/(d.width+1.0f),area.height/(d.height+1.0f));
            normW=d.width*unit; normH=d.height*unit;
            normX=area.x+(area.width-normW)/2; normY=area.y+(area.height-normH)/2;
            normalizedGeometry.clear(); recording=true;
            Renderer::tilePart(d,area);
            recording=false;
            matches();
        }
    }
    // Lock the VAB palette independently of UI theme colors. Grid coordinates
    // remain pixel-aligned and distinguish every fourth line at normal zoom.
    lines.clear(); gridLines.clear(); Renderer::workspace(view,{-100,-100,200,200});
    assert(equal(lastRectangleColor,Color{40,52,66,255}));
    assert(near(lastRectangle.x,-100) && near(lastRectangle.y,-100) && near(lastRectangle.width,200) && near(lastRectangle.height,200));
    bool minor=false,major=false;
    for(const auto& line:gridLines) {
        assert(near(line.width,1));
        assert(near(line.a.x,line.b.x) || near(line.a.y,line.b.y));
        assert(near(line.a.x,std::round(line.a.x)) && near(line.a.y,std::round(line.a.y)));
        minor|=equal(line.color,Color{47,61,75,255});
        major|=equal(line.color,Color{55,70,86,255});
        assert(equal(line.color,Color{47,61,75,255}) || equal(line.color,Color{55,70,86,255}));
    }
    assert(minor && major);
    for(const auto& line:lines) assert(!(near(line.a.y,line.b.y) && line.width>1)); // no ground barrier
    std::cout << "All catalog designs, white-pixel meshes, attachment faces and reflection passed\n";
}

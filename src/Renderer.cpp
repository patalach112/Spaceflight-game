#include "Renderer.hpp"
#include "UIComponents.hpp"
#include <algorithm>
#include <cmath>
namespace {
Color alpha(Color c, unsigned char a) { c.a = a; return c; }
void line(Vector2 a, Vector2 b, float thickness, Color c) { DrawLineEx(a,b,thickness,c); }
void shape(const PartDef& d, float x, float y, float w, float h, Color tint, bool selected) {
    const Color edge = selected ? ui::cyan : tint;
    Color fill = alpha(Color{54,66,75,255}, tint.a);
    Color detail = alpha(Color{133,153,163,255}, tint.a);
    float t = std::max(1.0f, w/60.0f);
    auto pt = [=](float px,float py) { return Vector2{x+px*w,y+py*h}; };
    auto segment = [&](float x1,float y1,float x2,float y2,Color c) { line(pt(x1,y1),pt(x2,y2),t,c); };
    if (d.category == Category::Pods) {
        Vector2 a=pt(.5f,.02f), b=pt(.14f,.70f), c=pt(.14f,.96f), e=pt(.86f,.96f), f=pt(.86f,.70f);
        DrawTriangle(a,b,f,fill); DrawTriangle(b,c,e,fill); DrawTriangle(b,e,f,fill);
        line(a,b,t,edge); line(b,c,t,edge); line(c,e,t,edge); line(e,f,t,edge); line(f,a,t,edge);
        segment(.32f,.57f,.68f,.57f,detail); segment(.23f,.73f,.77f,.73f,detail);
        DrawCircleV(pt(.5f,.42f), std::max(1.0f,w*.07f), alpha(ui::cyan,tint.a));
    } else if (d.category == Category::FuelTanks) {
        Rectangle body{x+w*.10f,y+h*.04f,w*.8f,h*.92f};
        DrawRectangleRec(body,fill); DrawRectangleLinesEx(body,t,edge);
        segment(.10f,.15f,.90f,.15f,detail); segment(.10f,.85f,.90f,.85f,detail);
        segment(.19f,.21f,.19f,.79f,detail); segment(.81f,.21f,.81f,.79f,detail);
        segment(.28f,.52f,.72f,.52f,alpha(ui::orange,tint.a));
    } else if (d.category == Category::Engines) {
        Rectangle top{x+w*.20f,y+h*.04f,w*.60f,h*.24f}; DrawRectangleRec(top,fill); DrawRectangleLinesEx(top,t,edge);
        Vector2 a=pt(.32f,.28f), b=pt(.15f,.91f), c=pt(.85f,.91f), e=pt(.68f,.28f);
        DrawTriangle(a,b,c,fill); DrawTriangle(a,c,e,fill);
        line(a,b,t,edge); line(b,c,t,edge); line(c,e,t,edge); line(e,a,t,edge);
        segment(.23f,.72f,.77f,.72f,detail); segment(.30f,.89f,.70f,.89f,alpha(ui::orange,tint.a));
    } else {
        Rectangle ring{x+w*.08f,y+h*.12f,w*.84f,h*.76f}; DrawRectangleRec(ring,fill); DrawRectangleLinesEx(ring,t,edge);
        segment(.2f,.50f,.8f,.50f,alpha(ui::orange,tint.a));
    }
}
}
Vector2 Renderer::toScreen(double gx, double gy, const Viewport& v) { return {v.originX+static_cast<float>(gx)*20*v.zoom,v.originY+static_cast<float>(gy)*20*v.zoom}; }
Vector2 Renderer::toGrid(Vector2 p, const Viewport& v) { return {(p.x-v.originX)/(20*v.zoom),(p.y-v.originY)/(20*v.zoom)}; }
void Renderer::workspace(const Viewport& v, Rectangle area) {
    DrawRectangleRec(area,ui::background);
    BeginScissorMode(static_cast<int>(area.x),static_cast<int>(area.y),static_cast<int>(area.width),static_cast<int>(area.height));
    float step=20*v.zoom; int stride=1; while(step*stride<12) stride*=2;
    Vector2 lo=toGrid({area.x,area.y},v), hi=toGrid({area.x+area.width,area.y+area.height},v);
    int startX=static_cast<int>(std::floor(lo.x/stride))*stride, endX=static_cast<int>(std::ceil(hi.x/stride))*stride;
    int startY=static_cast<int>(std::floor(lo.y/stride))*stride, endY=static_cast<int>(std::ceil(hi.y/stride))*stride;
    for(int x=startX;x<=endX;x+=stride) { float sx=toScreen(x,0,v).x; DrawLineV({sx,area.y},{sx,area.y+area.height},x%4==0?ui::line:Color{30,34,41,255}); }
    for(int y=startY;y<=endY;y+=stride) { float sy=toScreen(0,y,v).y; DrawLineV({area.x,sy},{area.x+area.width,sy},y%4==0?ui::line:Color{30,34,41,255}); }
    float axis=toScreen(0,0,v).x, floor=toScreen(0,0,v).y;
    DrawLineEx({axis,area.y},{axis,area.y+area.height},1.5f,Color{154,91,49,255});
    DrawLineEx({area.x,floor},{area.x+area.width,floor},2,Color{104,121,128,255});
    EndScissorMode();
}
void Renderer::part(const PartDef& d, GridPoint p, const Viewport& v, Color tint, bool selected) {
    Vector2 at=toScreen(p.x,p.y,v); shape(d,at.x,at.y,d.width*20*v.zoom,d.height*20*v.zoom,tint,selected);
}
void Renderer::tilePart(const PartDef& d, Rectangle r) {
    float unit=std::min(r.width/(d.width+1.0f),r.height/(d.height+1.0f));
    float w=d.width*unit,h=d.height*unit; shape(d,r.x+(r.width-w)/2,r.y+(r.height-h)/2,w,h,ui::text,false);
}

#include "Renderer.hpp"
#include "UIComponents.hpp"
#include <algorithm>
#include <cmath>
namespace {
Color withAlpha(Color c,unsigned char a) { c.a=a; return c; }
void shape(const PartDef& d,float x,float y,float w,float h,Color tint,bool selected) {
    const auto a=tint.a;
    Color dark=withAlpha({43,59,75,255},a), metal=withAlpha({168,183,192,255},a), highlight=withAlpha({215,224,228,255},a), shadow=withAlpha({93,113,129,255},a);
    Color seam=withAlpha({66,85,101,255},a), accent=withAlpha({93,178,216,255},a);
    auto pt=[=](float u,float v){return Vector2{x+u*w,y+v*h};};
    auto edge=[&](float x1,float y1,float x2,float y2,Color c=Color{43,59,75,255}) { if(a<255) c=tint; c.a=a; DrawLineEx(pt(x1,y1),pt(x2,y2),1.0f,c); };
    auto band=[&](float y0,float y1,Color left,Color right) { DrawRectangleGradientH(static_cast<int>(x),static_cast<int>(y+y0*h),std::max(1,static_cast<int>(std::ceil(w))),std::max(1,static_cast<int>(std::ceil((y1-y0)*h))),left,right); };
    if(d.category==Category::Pods) {
        Vector2 tip=pt(.5f,0), left=pt(0,.76f), bottomLeft=pt(0,1), bottomRight=pt(1,1), right=pt(1,.76f);
        DrawTriangle(tip,left,right,metal); DrawTriangle(left,bottomLeft,bottomRight,metal); DrawTriangle(left,bottomRight,right,metal);
        DrawTriangle(tip,left,pt(.25f,.76f),highlight); DrawTriangle(tip,pt(.72f,.76f),right,shadow);
        edge(.5f,0,0,.76f); edge(0,.76f,0,1); edge(0,1,1,1); edge(1,1,1,.76f); edge(1,.76f,.5f,0);
        edge(.13f,.70f,.87f,.70f,seam); edge(.02f,.89f,.98f,.89f,seam);
        DrawCircleV(pt(.5f,.48f),std::max(2.0f,w*.12f),dark);
        DrawCircleV(pt(.5f,.48f),std::max(1.0f,w*.09f),accent);
        edge(.43f,.45f,.53f,.41f,highlight);
    } else if(d.category==Category::FuelTanks) {
        band(0,1,highlight,shadow);
        DrawRectangleRec({x+w*.18f,y,w*.16f,h},Color{238,243,244,static_cast<unsigned char>(38*a/255)});
        edge(0,0,1,0); edge(0,1,1,1); edge(0,0,0,1); edge(1,0,1,1);
        edge(0,.12f,1,.12f,seam); edge(0,.16f,1,.16f,highlight);
        edge(0,.84f,1,.84f,seam); edge(0,.88f,1,.88f,highlight);
        edge(.10f,.18f,.10f,.82f,withAlpha({195,207,214,255},a));
        edge(.88f,.18f,.88f,.82f,seam);
        edge(.28f,.5f,.72f,.5f,accent);
    } else if(d.category==Category::Engines) {
        band(0,.20f,highlight,shadow); edge(0,0,1,0); edge(0,.20f,1,.20f); edge(0,0,0,.20f); edge(1,0,1,.20f);
        DrawRectangleRec({x+w*.27f,y+h*.20f,w*.46f,h*.18f},shadow);
        DrawCircleV(pt(.22f,.30f),std::max(1.0f,w*.07f),seam); DrawCircleV(pt(.78f,.30f),std::max(1.0f,w*.07f),seam);
        Vector2 p=pt(.32f,.34f),q=pt(.06f,1),r=pt(.94f,1),s=pt(.68f,.34f);
        DrawTriangle(p,q,r,metal); DrawTriangle(p,r,s,metal); DrawTriangle(p,q,pt(.38f,1),highlight); DrawTriangle(s,pt(.69f,1),r,shadow);
        edge(.32f,.34f,.06f,1); edge(.06f,1,.94f,1); edge(.94f,1,.68f,.34f); edge(.68f,.34f,.32f,.34f);
        edge(.11f,.88f,.89f,.88f,seam); edge(.35f,.42f,.65f,.42f,highlight);
    } else {
        band(0,1,highlight,shadow); edge(0,0,1,0); edge(0,1,1,1); edge(0,0,0,1); edge(1,0,1,1);
        edge(.05f,.5f,.95f,.5f,seam); edge(.12f,.25f,.88f,.25f,accent);
    }
    if(selected) DrawRectangleLinesEx({x,y,w,h},1.5f,withAlpha(ui::cyan,a));
}
}
Vector2 Renderer::toScreen(double gx,double gy,const Viewport& v) { return {v.originX+static_cast<float>(gx)*20*v.zoom,v.originY+static_cast<float>(gy)*20*v.zoom}; }
Vector2 Renderer::toGrid(Vector2 p,const Viewport& v) { return {(p.x-v.originX)/(20*v.zoom),(p.y-v.originY)/(20*v.zoom)}; }
void Renderer::workspace(const Viewport& v,Rectangle area) {
    DrawRectangleRec(area,ui::background);
    BeginScissorMode(static_cast<int>(area.x),static_cast<int>(area.y),static_cast<int>(area.width),static_cast<int>(area.height));
    float step=20*v.zoom; int stride=1; while(step*stride<12) stride*=2;
    Vector2 lo=toGrid({area.x,area.y},v),hi=toGrid({area.x+area.width,area.y+area.height},v);
    int sx=static_cast<int>(std::floor(lo.x/stride))*stride,ex=static_cast<int>(std::ceil(hi.x/stride))*stride;
    int sy=static_cast<int>(std::floor(lo.y/stride))*stride,ey=static_cast<int>(std::ceil(hi.y/stride))*stride;
    for(int x=sx;x<=ex;x+=stride) { float px=std::round(toScreen(x,0,v).x); DrawLineV({px,area.y},{px,area.y+area.height},x%4==0?ui::line:ui::grid); }
    for(int y=sy;y<=ey;y+=stride) { float py=std::round(toScreen(0,y,v).y); DrawLineV({area.x,py},{area.x+area.width,py},y%4==0?ui::line:ui::grid); }
    float axis=toScreen(0,0,v).x,floor=toScreen(0,0,v).y;
    DrawLineEx({axis,area.y},{axis,area.y+area.height},1.0f,Color{98,157,197,180});
    DrawLineEx({area.x,floor},{area.x+area.width,floor},1.5f,Color{155,192,213,220});
    EndScissorMode();
}
void Renderer::part(const PartDef& d,double x,double y,int rotation,const Viewport& v,Color tint,bool selected) {
    Vector2 screen=toScreen(x,y,v); float w=d.width*20*v.zoom,h=d.height*20*v.zoom;
    int r=normalizedRotation(rotation);
    Vector2 offset=screen;
    if(r==1) offset.x+=h; else if(r==2) { offset.x+=w; offset.y+=h; } else if(r==3) offset.y+=w;
    Camera2D camera{}; camera.offset=offset; camera.rotation=90.0f*r; camera.zoom=1.0f;
    BeginMode2D(camera); shape(d,0,0,w,h,tint,selected); EndMode2D();
}
void Renderer::tilePart(const PartDef& d,Rectangle r) {
    float unit=std::min(r.width/(d.width+1.0f),r.height/(d.height+1.0f));
    float w=d.width*unit,h=d.height*unit; shape(d,r.x+(r.width-w)/2,r.y+(r.height-h)/2,w,h,ui::text,false);
}

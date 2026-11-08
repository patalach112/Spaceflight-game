#include "Renderer.hpp"
#include "UIComponents.hpp"
#include <rlgl.h>
#include <algorithm>
#include <cmath>

namespace {
Color withAlpha(Color color, unsigned char alpha) { color.a = alpha; return color; }
Color blend(Color a, Color b, float t, unsigned char alpha) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto channel = [t](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(std::round(x + (static_cast<float>(y) - x)*t));
    };
    return {channel(a.r,b.r), channel(a.g,b.g), channel(a.b,b.b), alpha};
}
// Light falls across a cylindrical shell: a soft highlight at the left and
// a cooler shadow toward the right. All stops share exact boundary positions.
Color metalAt(float x, unsigned char alpha) {
    constexpr Color edge{139,157,169,255}, light{224,232,235,255};
    constexpr Color mid{180,196,205,255}, shadow{105,127,144,255};
    if (x < 0.22f) return blend(edge, light, x/0.22f, alpha);
    if (x < 0.62f) return blend(light, mid, (x-0.22f)/0.40f, alpha);
    return blend(mid, shadow, (x-0.62f)/0.38f, alpha);
}
void gradientBand(Rectangle rect, unsigned char alpha) {
    constexpr float stops[]{0.0f, 0.22f, 0.62f, 1.0f};
    for (int i=0; i<3; ++i) {
        float x0=stops[i], x1=stops[i+1];
        Color left=metalAt(x0,alpha), right=metalAt(x1,alpha);
        DrawRectangleGradientEx({rect.x+rect.width*x0,rect.y,rect.width*(x1-x0),rect.height},left,left,right,right);
    }
}
// A single batched, vertex-colored mesh avoids the hairline seams that appear
// when separately antialiased strips are composited over the canvas.
template<typename Top>
void shadedSilhouette(float x, float y, float width, float height, float from, float to, Top top, unsigned char alpha) {
    constexpr int slices=32;
    const Texture2D texture=GetShapesTexture();
    const Rectangle white=GetShapesTextureRectangle();
    const float u=(white.x+white.width*.5f)/texture.width;
    const float v=(white.y+white.height*.5f)/texture.height;
    auto vertex=[&](float px,float py,Color color) {
        rlColor4ub(color.r,color.g,color.b,color.a);
        rlTexCoord2f(u,v);
        rlVertex2f(px,py);
    };
    rlSetTexture(texture.id);
    rlBegin(RL_TRIANGLES);
    for (int i=0; i<slices; ++i) {
        float u0=from+(to-from)*i/slices, u1=from+(to-from)*(i+1)/slices;
        float x0=x+u0*width,x1=x+u1*width,y0=y+top(u0)*height,y1=y+top(u1)*height;
        Color left=metalAt(u0,alpha),right=metalAt(u1,alpha);
        vertex(x0,y0,left); vertex(x0,y+height,left); vertex(x1,y+height,right);
        vertex(x0,y0,left); vertex(x1,y+height,right); vertex(x1,y1,right);
    }
    rlEnd();
    rlSetTexture(0);
}
void shape(const PartDef& d, float x, float y, float w, float h, Color tint, bool selected) {
    const auto alpha=tint.a;
    const Color outline=withAlpha({42,59,76,255},alpha);
    const Color seam=withAlpha({68,87,104,255},alpha);
    const Color bright=withAlpha({225,235,239,255},alpha);
    const Color darkMetal=withAlpha({77,98,114,255},alpha);
    const Color glass=withAlpha({62,130,168,255},alpha);
    auto point=[=](float u,float v) { return Vector2{x+u*w,y+v*h}; };
    auto line=[&](float x1,float y1,float x2,float y2,Color color) {
        if(alpha<255 && color.r==outline.r && color.g==outline.g && color.b==outline.b) color=tint;
        color.a=alpha;
        DrawLineEx(point(x1,y1),point(x2,y2),1.0f,color);
    };
    auto frame=[&](float y0,float y1) {
        line(0,y0,1,y0,outline); line(0,y1,1,y1,outline);
        line(0,y0,0,y1,outline); line(1,y0,1,y1,outline);
    };
    const bool detailed=w>=26 && h>=18;
    if (d.category==Category::Pods) {
        auto roof=[](float u) { return 0.77f*std::abs(2.0f*u-1.0f); };
        shadedSilhouette(x,y,w,h,0,1,roof,alpha);
        line(.5f,0,0,.77f,outline); line(0,.77f,0,1,outline);
        line(0,1,1,1,outline); line(1,1,1,.77f,outline); line(1,.77f,.5f,0,outline);
        if (detailed) {
            line(.045f,.82f,.955f,.82f,seam);
            line(.045f,.855f,.955f,.855f,bright);
            line(.02f,.94f,.98f,.94f,seam);
            line(.18f,.70f,.35f,.39f,bright);
            line(.82f,.70f,.65f,.39f,seam);
            Vector2 center=point(.5f,.50f); float radius=std::min(w,h)*.125f;
            DrawCircleV(center,radius,outline);
            DrawCircleV(center,radius*.82f,bright);
            DrawCircleV(center,radius*.68f,darkMetal);
            DrawCircleV(center,radius*.53f,glass);
            DrawCircleLinesV(center,radius*.68f,outline);
            line(.46f,.46f,.52f,.435f,bright);
        }
    } else if (d.category==Category::FuelTanks) {
        gradientBand({x,y,w,h},alpha); frame(0,1);
        if (detailed) {
            // Paired ribs are inset from the attachment faces; the outer frame
            // remains the single shared outline when adjacent parts are flush.
            line(.015f,.125f,.985f,.125f,seam); line(.015f,.145f,.985f,.145f,bright);
            line(.015f,.855f,.985f,.855f,bright); line(.015f,.875f,.985f,.875f,seam);
            line(.12f,.17f,.12f,.83f,withAlpha({214,226,232,255},alpha));
            line(.88f,.17f,.88f,.83f,seam);
            line(.39f,.50f,.61f,.50f,darkMetal);
        }
    } else if (d.category==Category::Engines) {
        // The full-width mounting flange starts at y=0 and ends at the node
        // face, matching the tank/decoupler bounds exactly.
        gradientBand({x,y,w,h*.18f},alpha); frame(0,.18f);
        auto bellTop=[](float u) {
            float distance=std::abs(u-.5f);
            if(distance<=.18f) return .39f;
            return .39f+.61f*std::pow(std::clamp((distance-.18f)/.25f,0.0f,1.0f),.67f);
        };
        shadedSilhouette(x,y,w,h,.07f,.93f,bellTop,alpha);
        if (detailed) {
            // Turbopump housings and throat rings sit above, not across, the bell.
            DrawRectangleRec({x+w*.30f,y+h*.19f,w*.40f,h*.20f},darkMetal);
            line(.30f,.19f,.30f,.39f,outline); line(.70f,.19f,.70f,.39f,outline);
            line(.30f,.25f,.70f,.25f,bright); line(.30f,.30f,.70f,.30f,seam);
            DrawCircleV(point(.21f,.275f),std::max(1.5f,w*.065f),outline);
            DrawCircleV(point(.21f,.275f),std::max(1.0f,w*.043f),darkMetal);
            DrawCircleV(point(.79f,.275f),std::max(1.5f,w*.065f),outline);
            DrawCircleV(point(.79f,.275f),std::max(1.0f,w*.043f),darkMetal);
            line(.16f,.19f,.16f,.23f,seam); line(.84f,.19f,.84f,.23f,seam);
        }
        line(.32f,.39f,.68f,.39f,outline);
        constexpr int segments=12;
        for(int i=0;i<segments;++i) {
            float t0=static_cast<float>(i)/segments, t1=static_cast<float>(i+1)/segments;
            float r0=.18f+.25f*std::pow(t0,1.0f/.67f), r1=.18f+.25f*std::pow(t1,1.0f/.67f);
            line(.5f-r0,.39f+.61f*t0,.5f-r1,.39f+.61f*t1,outline);
            line(.5f+r0,.39f+.61f*t0,.5f+r1,.39f+.61f*t1,outline);
        }
        line(.07f,1,.93f,1,outline);
        if(detailed) { line(.11f,.94f,.89f,.94f,seam); line(.18f,.975f,.82f,.975f,darkMetal); }
    } else {
        gradientBand({x,y,w,h},alpha); frame(0,1);
        if(h>=8 && w>=26) {
            line(.04f,.32f,.96f,.32f,bright);
            line(.04f,.68f,.96f,.68f,seam);
            DrawCircleV(point(.17f,.50f),1.0f,outline);
            DrawCircleV(point(.83f,.50f),1.0f,outline);
        }
    }
    if(selected) DrawRectangleLinesEx({x,y,w,h},1.0f,withAlpha(ui::cyan,alpha));
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

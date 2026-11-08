#include "Renderer.hpp"
#include "UIComponents.hpp"
#include <rlgl.h>
#include <algorithm>
#include <cmath>

namespace {
Color withTint(Color color, Color tint) {
    return {static_cast<unsigned char>(color.r*static_cast<unsigned>(tint.r)/255),
            static_cast<unsigned char>(color.g*static_cast<unsigned>(tint.g)/255),
            static_cast<unsigned char>(color.b*static_cast<unsigned>(tint.b)/255),
            static_cast<unsigned char>(color.a*static_cast<unsigned>(tint.a)/255)};
}
Color blend(Color a, Color b, float t, Color tint) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto channel = [t](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(std::round(x + (static_cast<float>(y) - x)*t));
    };
    return withTint({channel(a.r,b.r), channel(a.g,b.g), channel(a.b,b.b), 255},tint);
}
// Light falls across a cylindrical shell: a soft highlight at the left and
// a cooler shadow toward the right. All stops share exact boundary positions.
Color metalAt(float x, Color tint) {
    constexpr Color edge{96,115,138,255}, light{166,181,193,255};
    constexpr Color mid{138,155,176,255}, shadow{96,115,138,255};
    if (x < 0.22f) return blend(edge, light, x/0.22f, tint);
    if (x < 0.62f) return blend(light, mid, (x-0.22f)/0.40f, tint);
    return blend(mid, shadow, (x-0.62f)/0.38f, tint);
}
void gradientBand(Rectangle rect, Color tint) {
    constexpr float stops[]{0.0f, 0.22f, 0.62f, 1.0f};
    for (int i=0; i<3; ++i) {
        float x0=stops[i], x1=stops[i+1];
        Color left=metalAt(x0,tint), right=metalAt(x1,tint);
        DrawRectangleGradientEx({rect.x+rect.width*x0,rect.y,rect.width*(x1-x0),rect.height},left,left,right,right);
    }
}
// A single batched, vertex-colored mesh avoids the hairline seams that appear
// when separately antialiased strips are composited over the canvas.
template<typename Top>
void shadedSilhouette(float x, float y, float width, float height, float from, float to, Top top, Color tint) {
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
        Color left=metalAt(u0,tint),right=metalAt(u1,tint);
        vertex(x0,y0,left); vertex(x0,y+height,left); vertex(x1,y+height,right);
        vertex(x0,y0,left); vertex(x1,y+height,right); vertex(x1,y1,right);
    }
    rlEnd();
    rlSetTexture(0);
}
void shape(const PartDef& d, float x, float y, float w, float h, Color tint, bool selected) {
    const auto alpha=tint.a;
    const Color outline=withTint({30,44,58,255},tint);
    const Color seam=withTint({68,87,104,255},tint);
    const Color bright=withTint({190,205,216,255},tint);
    const Color darkMetal=withTint({77,98,114,255},tint);
    const Color glass=withTint({62,130,168,255},tint);
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
    if (d.shape==PartShape::Capsule || d.shape==PartShape::NoseCone) {
        float shoulder=d.shape==PartShape::Capsule?.77f:.90f;
        auto roof=[shoulder](float u) { return shoulder*std::abs(2.0f*u-1.0f); };
        shadedSilhouette(x,y,w,h,0,1,roof,tint);
        line(.5f,0,0,shoulder,outline); line(0,shoulder,0,1,outline);
        line(0,1,1,1,outline); line(1,1,1,shoulder,outline); line(1,shoulder,.5f,0,outline);
        if (detailed && d.shape==PartShape::Capsule) {
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
        } else if(detailed) {
            line(.015f,.94f,.985f,.94f,seam);
        }
    } else if (d.shape==PartShape::Probe) {
        gradientBand({x,y,w,h},tint); frame(0,1);
        if(detailed) {
            DrawRectangleRec({x+w*.18f,y+h*.22f,w*.64f,h*.56f},darkMetal);
            line(.18f,.22f,.82f,.22f,outline); line(.18f,.78f,.82f,.78f,outline);
            line(.18f,.22f,.18f,.78f,outline); line(.82f,.22f,.82f,.78f,outline);
            line(.26f,.35f,.74f,.35f,bright); line(.26f,.62f,.74f,.62f,seam);
            DrawCircleV(point(.5f,.49f),std::max(1.0f,std::min(w,h)*.075f),glass);
        }
    } else if (d.shape==PartShape::Tank) {
        gradientBand({x,y,w,h},tint); frame(0,1);
        if (detailed) {
            // Paired ribs are inset from the attachment faces; the outer frame
            // remains the single shared outline when adjacent parts are flush.
            line(.015f,.125f,.985f,.125f,seam); line(.015f,.145f,.985f,.145f,bright);
            line(.015f,.855f,.985f,.855f,bright); line(.015f,.875f,.985f,.875f,seam);
            line(.12f,.17f,.12f,.83f,withTint({173,191,205,255},tint));
            line(.88f,.17f,.88f,.83f,seam);
            line(.39f,.50f,.61f,.50f,darkMetal);
        }
    } else if (d.shape==PartShape::Engine) {
        // The full-width mounting flange starts at y=0 and ends at the node
        // face, matching the tank/decoupler bounds exactly.
        gradientBand({x,y,w,h*.18f},tint); frame(0,.18f);
        const bool wideBell=d.id==2 || d.id==9;
        float throat=wideBell?.18f:.13f, rim=wideBell?.43f:.46f;
        auto bellTop=[=](float u) {
            float distance=std::abs(u-.5f);
            if(distance<=throat) return .39f;
            return .39f+.61f*std::pow(std::clamp((distance-throat)/(rim-throat),0.0f,1.0f),.67f);
        };
        shadedSilhouette(x,y,w,h,.5f-rim,.5f+rim,bellTop,tint);
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
        line(.5f-throat,.39f,.5f+throat,.39f,outline);
        constexpr int segments=12;
        for(int i=0;i<segments;++i) {
            float t0=static_cast<float>(i)/segments, t1=static_cast<float>(i+1)/segments;
            float r0=throat+(rim-throat)*std::pow(t0,1.0f/.67f), r1=throat+(rim-throat)*std::pow(t1,1.0f/.67f);
            line(.5f-r0,.39f+.61f*t0,.5f-r1,.39f+.61f*t1,outline);
            line(.5f+r0,.39f+.61f*t0,.5f+r1,.39f+.61f*t1,outline);
        }
        line(.5f-rim,1,.5f+rim,1,outline);
        if(detailed) { line(.5f-rim+.04f,.94f,.5f+rim-.04f,.94f,seam); line(.5f-rim+.11f,.975f,.5f+rim-.11f,.975f,darkMetal); }
    } else if(d.shape==PartShape::SideNose) {
        auto roof=[](float u) { return .80f*u; };
        shadedSilhouette(x,y,w,h,0,1,roof,tint);
        line(0,0,1,.80f,outline); line(1,.80f,1,1,outline);
        line(1,1,0,1,outline); line(0,1,0,0,outline);
        if(detailed) line(.02f,.94f,.98f,.94f,seam);
    } else if(d.shape==PartShape::Parachute) {
        auto roof=[](float u) { return .32f*std::abs(2*u-1); };
        shadedSilhouette(x,y,w,h,0,1,roof,tint);
        line(0,.32f,.5f,0,outline); line(.5f,0,1,.32f,outline);
        line(0,.32f,0,1,outline); line(1,.32f,1,1,outline); line(0,1,1,1,outline);
        if(detailed) line(.08f,.76f,.92f,.76f,seam);
    } else if(d.shape==PartShape::LandingLeg) {
        DrawRectangleRec({x,y+h*.34f,w*.20f,h*.32f},darkMetal);
        line(0,.34f,.20f,.34f,outline); line(0,.66f,.20f,.66f,outline);
        line(.20f,.34f,.20f,.66f,outline);
        DrawTriangle(point(.16f,.39f),point(.40f,.80f),point(.61f,.80f),withTint({116,136,154,255},tint));
        DrawTriangle(point(.16f,.39f),point(.61f,.80f),point(.31f,.39f),withTint({116,136,154,255},tint));
        line(.16f,.39f,.40f,.80f,outline); line(.31f,.39f,.61f,.80f,outline);
        line(.40f,.80f,.61f,.80f,outline); line(.61f,.80f,.85f,.94f,outline);
        DrawRectangleRec({x+w*.53f,y+h*.94f,w*.47f,h*.06f},darkMetal);
        line(.53f,.94f,1,.94f,outline); line(.53f,1,1,1,outline);
    } else if(d.shape==PartShape::Rcs) {
        gradientBand({x,y+h*.25f,w*.54f,h*.50f},tint);
        line(0,.25f,.54f,.25f,outline); line(0,.75f,.54f,.75f,outline);
        line(0,.25f,0,.75f,outline); line(.54f,.25f,.54f,.75f,outline);
        DrawTriangle(point(.54f,.35f),point(.54f,.65f),point(.90f,.80f),darkMetal);
        DrawTriangle(point(.54f,.35f),point(.90f,.80f),point(.90f,.20f),darkMetal);
        line(.54f,.35f,.90f,.20f,outline); line(.54f,.65f,.90f,.80f,outline);
        line(.90f,.20f,.90f,.80f,outline);
        line(.84f,.32f,.84f,.68f,bright);
    } else {
        gradientBand({x,y,w,h},tint); frame(0,1);
        if(d.shape==PartShape::HeatShield) {
            DrawRectangleRec({x,y+h*.48f,w,h*.52f},darkMetal);
            line(0,.48f,1,.48f,outline); line(0,1,1,1,outline);
            if(detailed) line(.08f,.73f,.92f,.73f,seam);
        } else if(d.shape==PartShape::DockingPort) {
            DrawRectangleRec({x+w*.24f,y,w*.52f,h*.26f},darkMetal);
            line(.24f,.26f,.76f,.26f,outline);
            if(detailed) line(.1f,.75f,.9f,.75f,seam);
        } else if(d.shape==PartShape::Truss && detailed) {
            line(.04f,.12f,.49f,.88f,outline); line(.04f,.88f,.49f,.12f,outline);
            line(.51f,.12f,.96f,.88f,outline); line(.51f,.88f,.96f,.12f,outline);
            line(.5f,.05f,.5f,.95f,seam);
        } else if(d.shape==PartShape::SideSeparator && detailed) {
            line(.30f,.08f,.30f,.92f,seam); line(.70f,.08f,.70f,.92f,seam);
            DrawCircleV(point(.5f,.25f),1.2f,outline); DrawCircleV(point(.5f,.75f),1.2f,outline);
        } else if(h>=8 && w>=26) {
            line(.04f,.32f,.96f,.32f,bright); line(.04f,.68f,.96f,.68f,seam);
        }
    }
    if(selected) DrawRectangleLinesEx({x,y,w,h},1.0f,withTint(ui::cyan,tint));
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
    float axis=toScreen(0,0,v).x;
    DrawLineEx({axis,area.y},{axis,area.y+area.height},1.0f,Color{98,157,197,180});
    EndScissorMode();
}
void Renderer::part(const PartDef& d,double x,double y,int rotation,const Viewport& v,Color tint,bool selected,bool flipped) {
    Vector2 screen=toScreen(x,y,v); float w=d.width*20*v.zoom,h=d.height*20*v.zoom;
    int r=normalizedRotation(rotation);
    Vector2 offset=screen;
    if(r==1) offset.x+=h; else if(r==2) { offset.x+=w; offset.y+=h; } else if(r==3) offset.y+=w;
    Camera2D camera{}; camera.offset=offset; camera.rotation=90.0f*r; camera.zoom=1.0f;
    BeginMode2D(camera);
    if(flipped) {
        // Reflection reverses triangle winding. Flush around the temporary
        // culling change so both regular and reflected batches remain visible.
        rlDrawRenderBatchActive(); rlDisableBackfaceCulling();
        rlPushMatrix(); rlTranslatef(w,0,0); rlScalef(-1,1,1);
    }
    shape(d,0,0,w,h,tint,selected);
    if(flipped) { rlDrawRenderBatchActive(); rlPopMatrix(); rlEnableBackfaceCulling(); }
    EndMode2D();
}
void Renderer::tilePart(const PartDef& d,Rectangle r) {
    float unit=std::min(r.width/(d.width+1.0f),r.height/(d.height+1.0f));
    float w=d.width*unit,h=d.height*unit; shape(d,r.x+(r.width-w)/2,r.y+(r.height-h)/2,w,h,ui::text,false);
}

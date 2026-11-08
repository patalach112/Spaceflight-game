#include "Renderer.hpp"
#include "UIComponents.hpp"
#include <rlgl.h>
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <stdexcept>

namespace {
Color tinted(Color c, Color t) {
    return {static_cast<unsigned char>(c.r*static_cast<unsigned>(t.r)/255),
            static_cast<unsigned char>(c.g*static_cast<unsigned>(t.g)/255),
            static_cast<unsigned char>(c.b*static_cast<unsigned>(t.b)/255),
            static_cast<unsigned char>(c.a*static_cast<unsigned>(t.a)/255)};
}
Color mix(Color a, Color b, float t, Color tint) {
    t=std::clamp(t,0.0f,1.0f);
    auto component=[t](unsigned char x,unsigned char y) {
        return static_cast<unsigned char>(std::round(x+(static_cast<float>(y)-x)*t));
    };
    return tinted({component(a.r,b.r),component(a.g,b.g),component(a.b,b.b),255},tint);
}
struct MaterialProfile { Color edge, light, mid; float highlight, shadow; };
// Individual finishes keep even similarly sized tank and structural parts from
// inheriting one identical strip texture. Values are deliberately muted so the
// line work remains visible at catalog scale.
MaterialProfile materialFor(int id) {
    static constexpr MaterialProfile profiles[] = {
        {{89,104,120,255},{190,201,208,255},{151,166,179,255},.27f,.69f},
        {{104,109,113,255},{231,232,230,255},{177,182,183,255},.22f,.63f},
        {{58,64,70,255},{149,155,156,255},{100,108,112,255},.33f,.64f},
        {{73,87,100,255},{160,172,180,255},{124,138,149,255},.18f,.71f},
        {{65,82,99,255},{163,181,194,255},{116,139,158,255},.30f,.58f},
        {{108,111,113,255},{237,237,234,255},{184,188,188,255},.24f,.65f},
        {{105,110,112,255},{234,234,232,255},{180,185,185,255},.20f,.60f},
        {{101,106,111,255},{227,229,228,255},{173,179,182,255},.26f,.72f},
        {{71,76,80,255},{190,190,183,255},{124,130,128,255},.29f,.66f},
        {{52,56,60,255},{144,151,153,255},{90,98,103,255},.36f,.62f},
        {{67,72,78,255},{207,210,207,255},{130,140,146,255},.25f,.67f},
        {{75,73,69,255},{194,189,178,255},{133,130,123,255},.31f,.57f},
        {{91,106,120,255},{195,203,208,255},{147,161,173,255},.19f,.68f},
        {{66,80,94,255},{165,179,190,255},{115,134,147,255},.28f,.70f},
        {{61,79,95,255},{161,179,191,255},{113,135,151,255},.21f,.59f},
        {{66,78,90,255},{158,170,177,255},{110,126,138,255},.32f,.65f},
        {{78,99,117,255},{174,191,203,255},{127,150,166,255},.23f,.74f},
        {{68,88,105,255},{168,187,200,255},{119,143,158,255},.35f,.62f},
        {{95,107,119,255},{196,204,209,255},{151,164,174,255},.16f,.61f},
        {{97,112,124,255},{185,197,204,255},{143,160,172,255},.25f,.75f},
        {{61,70,77,255},{150,158,159,255},{104,116,121,255},.34f,.70f},
        {{80,96,109,255},{182,195,204,255},{132,151,164,255},.20f,.66f}
    };
    if(id<0 || id>=static_cast<int>(sizeof(profiles)/sizeof(profiles[0])))
        throw std::logic_error("Part is missing its material profile");
    return profiles[id];
}
Color metalAt(float u,Color tint,const MaterialProfile& p) {
    if(u<p.highlight) return mix(p.edge,p.light,u/p.highlight,tint);
    if(u<p.shadow) return mix(p.light,p.mid,(u-p.highlight)/(p.shadow-p.highlight),tint);
    return mix(p.mid,p.edge,(u-p.shadow)/(1-p.shadow),tint);
}
// Mesh strips use RLGL's own white pixel, not the font/shape atlas or any
// previous part texture. QUADS keep the texture and primitive mode stable as
// Raylib switches between text, standard shapes and these custom silhouettes.
template<class Top,class Bottom>
void shell(float x,float y,float w,float h,float from,float to,Top top,Bottom bottom,Color tint,Color tone,const MaterialProfile& material) {
    constexpr int slices=32;
    rlSetTexture(rlGetTextureIdDefault());
    rlBegin(RL_QUADS);
    auto vertex=[](float px,float py,Color c) {
        rlColor4ub(c.r,c.g,c.b,c.a); rlTexCoord2f(.5f,.5f); rlVertex2f(px,py);
    };
    for(int i=0;i<slices;++i) {
        float a=from+(to-from)*i/slices,b=from+(to-from)*(i+1)/slices;
        Color ca=tinted(metalAt(a,tint,material),tone), cb=tinted(metalAt(b,tint,material),tone);
        vertex(x+w*a,y+h*top(a),ca); vertex(x+w*a,y+h*bottom(a),ca);
        vertex(x+w*b,y+h*bottom(b),cb); vertex(x+w*b,y+h*top(b),cb);
    }
    rlEnd(); rlSetTexture(0);
}
struct Canvas {
    float x,y,w,h;
    Color tint;
    MaterialProfile material;
    Color ink() const { return tint.a<255?tint:tinted({30,44,58,255},tint); }
    Color seam() const { return tinted({65,83,99,255},tint); }
    Color bright() const { return tinted({190,205,216,255},tint); }
    Color dark() const { return tinted({62,77,90,255},tint); }
    Color black() const { return tinted({28,38,47,255},tint); }
    Vector2 p(float u,float v) const { return {x+u*w,y+v*h}; }
    void line(float u,float v,float a,float b,Color c,float width=1) const { DrawLineEx(p(u,v),p(a,b),width,c); }
    void rect(float u,float v,float a,float b,Color c) const { DrawRectangleRec({x+u*w,y+v*h,w*a,h*b},c); }
    void circle(float u,float v,float radius,Color c) const { DrawCircleV(p(u,v),std::min(w,h)*radius,c); }
    void frame(float x0,float y0,float x1,float y1,Color c) const {
        line(x0,y0,x1,y0,c); line(x1,y0,x1,y1,c); line(x1,y1,x0,y1,c); line(x0,y1,x0,y0,c);
    }
    void poly(std::initializer_list<Vector2> vertices,Color c) const {
        if(vertices.size()<3) return;
        auto it=vertices.begin(); Vector2 a=p(it->x,it->y),b=p((it+1)->x,(it+1)->y);
        for(it+=2;it!=vertices.end();++it) {
            Vector2 q=p(it->x,it->y), b2=b, q2=q;
            // Raylib triangles use counterclockwise winding in screen space.
            if((b2.x-a.x)*(q2.y-a.y)-(b2.y-a.y)*(q2.x-a.x)>0) std::swap(b2,q2);
            DrawTriangle(a,b2,q2,c); b=q;
        }
    }
    void band(float y0,float y1) const {
        const float stops[]{0,material.highlight,material.shadow,1};
        for(int i=0;i<3;++i) {
            float a=stops[i],b=stops[i+1]; Color ca=metalAt(a,tint,material),cb=metalAt(b,tint,material);
            DrawRectangleGradientEx({x+w*a,y+h*y0,w*(b-a),h*(y1-y0)},ca,ca,cb,cb);
        }
    }
    template<class Top,class Bottom>
    void skin(float from,float to,Top top,Bottom bottom,Color tone={255,255,255,255}) const { shell(x,y,w,h,from,to,top,bottom,tint,tone,material); }
    template<class Curve>
    void curve(float from,float to,Curve fn,Color c) const {
        constexpr int n=20;
        for(int i=0;i<n;++i) { float a=from+(to-from)*i/n,b=from+(to-from)*(i+1)/n; line(a,fn(a),b,fn(b),c); }
    }
    // Flat, foreshortened ellipse built from convex triangles, so the nozzle
    // stays in the same local coordinates as the rest of the rotated part.
    void ellipse(float u,float v,float rx,float ry,Color color) const {
        constexpr int segments=32;
        constexpr float tau=6.28318530718f;
        for(int i=0;i<segments;++i) {
            float a=tau*i/segments,b=tau*(i+1)/segments;
            poly({{u,v},{u+rx*std::cos(a),v+ry*std::sin(a)},
                  {u+rx*std::cos(b),v+ry*std::sin(b)}},color);
        }
    }
    void plate(float u0,float v0,float u1,float v1) const {
        const float stops[]{0,material.highlight,material.shadow,1};
        for(int i=0;i<3;++i) {
            float a=stops[i],b=stops[i+1];
            Color ca=metalAt(a,tint,material),cb=metalAt(b,tint,material);
            DrawRectangleGradientEx({x+w*(u0+(u1-u0)*a),y+h*v0,w*(u1-u0)*(b-a),h*(v1-v0)},ca,ca,cb,cb);
        }
        frame(u0,v0,u1,v1,ink());
    }
    void outline(std::initializer_list<Vector2> points,Color color) const {
        if(points.size()<2) return;
        auto first=points.begin(), previous=first;
        for(auto it=first+1;it!=points.end();++it) { line(previous->x,previous->y,it->x,it->y,color); previous=it; }
        line(previous->x,previous->y,first->x,first->y,color);
    }
    void bolt(float u,float v) const {
        circle(u,v,.035f,ink()); circle(u,v,.016f,bright());
    }
};

void capsule(const Canvas& c) {
    // Blunt crown, swept pressure shell and a flush attachment skirt. The
    // shell deliberately has no porthole or inset window.
    auto roof=[](float u) {
        float d=std::abs(2*u-1);
        return d<=.5f ? 0.0f : .78f*std::pow((d-.5f)*2,1.05f);
    };
    c.skin(0,1,roof,[](float){return 1.0f;});
    c.curve(0,1,roof,c.ink());
    c.line(0,.78f,0,1,c.ink()); c.line(1,.78f,1,1,c.ink()); c.line(0,1,1,1,c.ink());
    c.line(.01f,.82f,.99f,.82f,c.seam());
    c.line(.025f,.86f,.975f,.86f,c.bright());
    c.line(.04f,.94f,.96f,.94f,c.seam());
    c.line(.12f,.68f,.27f,.18f,c.seam());
    c.line(.88f,.68f,.73f,.18f,c.seam());
    c.line(.29f,.035f,.71f,.035f,c.bright());
}
void probe(const Canvas& c) {
    // Broad flat mating lips with a recessed middle body; no generic pod or
    // engine artwork is reused here.
    c.plate(0,0,1,1);
    c.poly({{0,.20f},{.10f,.20f},{.10f,.80f},{0,.80f}},c.dark());
    c.poly({{.90f,.20f},{1,.20f},{1,.80f},{.90f,.80f}},c.dark());
    c.line(0,.20f,.10f,.20f,c.ink()); c.line(0,.80f,.10f,.80f,c.ink());
    c.line(.90f,.20f,1,.20f,c.ink()); c.line(.90f,.80f,1,.80f,c.ink());
    c.line(.10f,.20f,.10f,.80f,c.ink()); c.line(.90f,.20f,.90f,.80f,c.ink());
    c.line(.10f,.15f,.90f,.15f,c.bright()); c.line(.10f,.85f,.90f,.85f,c.seam());
    c.line(.18f,.32f,.82f,.32f,c.seam()); c.line(.18f,.68f,.82f,.68f,c.bright());
    c.line(.22f,.47f,.78f,.47f,c.seam()); c.line(.22f,.52f,.65f,.52f,c.seam());
    c.bolt(.16f,.50f); c.bolt(.84f,.50f);
}
// Separate tank designs: the cylindrical fill and mating edge are common
// primitives, but each size has its own seam and panel layout.
void tankBody(const Canvas& c) { c.band(0,1); c.frame(0,0,1,1,c.ink()); }
void tank4x2(const Canvas& c) {
    tankBody(c);
    c.line(.02f,.13f,.98f,.13f,c.bright()); c.line(.02f,.87f,.98f,.87f,c.seam());
    c.line(.13f,.20f,.13f,.80f,c.bright()); c.line(.87f,.20f,.87f,.80f,c.seam());
    c.line(.18f,.50f,.82f,.50f,tinted({153,169,182,255},c.tint));
}
void tank4x4(const Canvas& c) {
    tankBody(c);
    c.line(.02f,.065f,.98f,.065f,c.bright()); c.line(.02f,.935f,.98f,.935f,c.seam());
    c.line(.02f,.50f,.98f,.50f,c.seam()); c.line(.02f,.52f,.98f,.52f,c.bright());
    c.line(.10f,.11f,.10f,.44f,c.bright()); c.line(.90f,.56f,.90f,.89f,c.seam());
}
void tank4x6(const Canvas& c) {
    tankBody(c);
    c.line(.02f,.045f,.98f,.045f,c.bright()); c.line(.02f,.955f,.98f,.955f,c.seam());
    for(float y:{.333f,.667f}) {
        c.line(.02f,y-.008f,.98f,y-.008f,c.seam()); c.line(.02f,y+.008f,.98f,y+.008f,c.bright());
    }
    c.line(.11f,.08f,.11f,.91f,c.bright()); c.line(.89f,.08f,.89f,.91f,c.seam());
}
void tank4x8(const Canvas& c) {
    tankBody(c);
    c.line(.02f,.035f,.98f,.035f,c.bright()); c.line(.02f,.965f,.98f,.965f,c.seam());
    for(float y:{.25f,.50f,.75f}) {
        c.line(.02f,y-.006f,.98f,y-.006f,c.seam()); c.line(.02f,y+.006f,.98f,y+.006f,c.bright());
    }
    c.line(.075f,.06f,.075f,.94f,c.seam()); c.line(.16f,.06f,.16f,.94f,c.bright());
    c.line(.84f,.06f,.84f,.94f,c.seam()); c.line(.925f,.06f,.925f,.94f,c.seam());
}
void tank(const Canvas& c,int id) {
    switch(id) {
        case 5: tank4x2(c); break; case 6: tank4x4(c); break;
        case 1: tank4x6(c); break; case 7: tank4x8(c); break;
        default: throw std::logic_error("Tank is missing its vector design");
    }
}
// Each engine specifies an independent, hand-authored radial station profile.
// The shared routine only rasterizes those contours; it does not generate a
// cone from one exponent or alter geometry based on catalog size.
struct BellStation { float y, halfWidth, light; };
void engineBell(const Canvas& c,std::initializer_list<BellStation> stations) {
    constexpr int slices=24;
    auto vertex=[&](const BellStation& s,float t) {
        Color base=metalAt(t,c.tint,c.material);
        auto shade=[&](unsigned char v) { return static_cast<unsigned char>(std::round(v*s.light)); };
        rlColor4ub(shade(base.r),shade(base.g),shade(base.b),base.a);
        rlTexCoord2f(.5f,.5f); rlVertex2f(c.x+c.w*(.5f+(2*t-1)*s.halfWidth),c.y+c.h*s.y);
    };
    rlSetTexture(rlGetTextureIdDefault()); rlBegin(RL_QUADS);
    for(auto row=stations.begin();row+1!=stations.end();++row) {
        for(int i=0;i<slices;++i) {
            float a=static_cast<float>(i)/slices,b=static_cast<float>(i+1)/slices;
            vertex(*row,a); vertex(*(row+1),a); vertex(*(row+1),b); vertex(*row,b);
        }
    }
    rlEnd(); rlSetTexture(0);
    for(auto row=stations.begin();row+1!=stations.end();++row) {
        c.line(.5f-row->halfWidth,row->y,.5f-(row+1)->halfWidth,(row+1)->y,c.ink());
        c.line(.5f+row->halfWidth,row->y,.5f+(row+1)->halfWidth,(row+1)->y,c.ink());
    }
}
void exitRim(const Canvas& c,float width,float y,float depth) {
    c.ellipse(.5f,y,width,depth,c.ink());
    c.ellipse(.5f,y-.002f,width*.96f,depth*.78f,tinted({135,147,153,255},c.tint));
    c.ellipse(.5f,y-.004f,width*.87f,depth*.56f,c.black());
    c.ellipse(.5f,y-.008f,width*.59f,depth*.29f,tinted({49,58,64,255},c.tint));
    c.line(.5f-width,y+depth,.5f+width,y+depth,c.ink());
}
void throat(const Canvas& c,float y,float width,float depth) {
    c.ellipse(.5f,y,width,depth,c.ink());
    c.ellipse(.5f,y,width*.66f,depth*.59f,c.black());
}
// A compact rounded pressure chamber, separately shaded from the dark bell.
// The caller chooses its placement and proportions; the surrounding hardware
// remains specific to each engine.
void chamber(const Canvas& c,float x,float y,float w,float h) {
    MaterialProfile finish=c.material;
    auto brighter=[](unsigned char v,int delta) { return static_cast<unsigned char>(std::min(255,static_cast<int>(v)+delta)); };
    finish.edge={brighter(finish.edge.r,32),brighter(finish.edge.g,32),brighter(finish.edge.b,32),255};
    finish.light={brighter(finish.light.r,36),brighter(finish.light.g,36),brighter(finish.light.b,36),255};
    finish.mid={brighter(finish.mid.r,38),brighter(finish.mid.g,38),brighter(finish.mid.b,38),255};
    Canvas body{c.x+c.w*x,c.y+c.h*y,c.w*w,c.h*h,c.tint,finish};
    auto top=[](float u){float d=2*u-1;return .20f*d*d;};
    auto bottom=[](float u){float d=2*u-1;return .98f-.22f*d*d;};
    body.skin(0,1,top,bottom); body.curve(0,1,top,body.ink()); body.curve(0,1,bottom,body.ink());
    body.line(0,.20f,0,.76f,body.ink()); body.line(1,.20f,1,.76f,body.ink());
}
void valiant(const Canvas& c) {
    // Slender upper-stage chamber, open triangular support and long nozzle.
    c.plate(0,0,1,.105f);
    c.line(.06f,.075f,.94f,.075f,c.bright());
    c.poly({{.18f,.105f},{.24f,.105f},{.37f,.38f},{.32f,.38f}},c.dark());
    c.poly({{.76f,.105f},{.82f,.105f},{.68f,.38f},{.63f,.38f}},c.dark());
    c.line(.18f,.105f,.32f,.38f,c.ink()); c.line(.82f,.105f,.68f,.38f,c.ink());
    chamber(c,.35f,.11f,.30f,.28f);
    c.plate(.36f,.20f,.64f,.27f);
    c.line(.40f,.31f,.44f,.37f,c.bright());
    c.line(.38f,.25f,.62f,.25f,c.bright());
    engineBell(c,{{.37f,.095f,.92f},{.48f,.13f,1.0f},{.70f,.18f,.95f},{.88f,.25f,.88f},{.96f,.30f,.84f}});
    c.line(.29f,.79f,.71f,.79f,c.seam());
    throat(c,.375f,.073f,.019f); exitRim(c,.29f,.965f,.029f);
}
void titan(const Canvas& c) {
    // Heavy mount, broad chamber, exposed gas generator and small vernier.
    c.plate(0,0,1,.13f); c.line(.03f,.095f,.97f,.095f,c.bright());
    for(float x:{.12f,.30f,.70f,.88f}) c.bolt(x,.055f);
    chamber(c,.30f,.13f,.40f,.26f);
    c.plate(.25f,.22f,.75f,.28f);
    c.line(.37f,.31f,.43f,.37f,c.bright());
    c.line(.16f,.12f,.22f,.35f,c.dark(),2); c.line(.84f,.12f,.78f,.35f,c.dark(),2);
    c.plate(.06f,.17f,.20f,.37f);
    c.poly({{.07f,.37f},{.19f,.37f},{.17f,.44f},{.09f,.44f}},c.dark());
    c.outline({{.07f,.37f},{.19f,.37f},{.17f,.44f},{.09f,.44f}},c.ink());
    c.line(.08f,.44f,.18f,.44f,c.black(),2);
    c.plate(.80f,.19f,.91f,.32f);
    c.line(.81f,.32f,.70f,.39f,c.seam(),2);
    engineBell(c,{{.37f,.14f,.88f},{.46f,.20f,.93f},{.58f,.27f,.98f},{.72f,.34f,.87f},{.86f,.40f,.94f},{.96f,.44f,.80f}});
    for(float y:{.46f,.58f,.72f,.86f}) {
        float r=y==.46f?.20f:(y==.58f?.27f:(y==.72f?.34f:.40f));
        c.line(.5f-r+.01f,y,.5f+r-.01f,y,c.seam());
    }
    throat(c,.38f,.105f,.024f); exitRim(c,.43f,.965f,.029f);
}
void hawk(const Canvas& c) {
    // First-stage steep bell with a squat gimbal and crossed side braces.
    c.plate(0,0,1,.12f);
    c.bolt(.14f,.055f); c.bolt(.86f,.055f);
    c.line(.17f,.12f,.32f,.37f,c.seam(),2); c.line(.83f,.12f,.68f,.37f,c.seam(),2);
    chamber(c,.29f,.11f,.42f,.21f);
    c.plate(.27f,.20f,.73f,.26f);
    c.poly({{.09f,.13f},{.16f,.13f},{.18f,.34f},{.11f,.34f}},c.dark());
    c.outline({{.09f,.13f},{.16f,.13f},{.18f,.34f},{.11f,.34f}},c.ink());
    c.line(.10f,.34f,.16f,.39f,c.seam(),2);
    engineBell(c,{{.29f,.11f,.91f},{.38f,.17f,.99f},{.54f,.26f,.92f},{.77f,.36f,.85f},{.96f,.41f,.78f}});
    for(const auto& rib: {Vector2{.39f,.176f}, {.46f,.215f}, {.54f,.26f}, {.62f,.295f},
                          {.70f,.33f}, {.78f,.363f}, {.86f,.384f}, {.92f,.40f}})
        c.line(.5f-rib.y+.01f,rib.x,.5f+rib.y-.01f,rib.x,c.seam());
    throat(c,.30f,.08f,.021f); exitRim(c,.40f,.965f,.029f);
}
void frontier(const Canvas& c) {
    // Long vacuum bell, bright central chamber, unequal side plumbing.
    c.plate(0,0,1,.09f); c.line(.02f,.065f,.98f,.065f,c.bright());
    chamber(c,.34f,.10f,.32f,.24f);
    c.plate(.32f,.16f,.68f,.21f);
    c.line(.39f,.24f,.46f,.32f,c.bright());
    c.plate(.10f,.12f,.22f,.24f); c.line(.16f,.24f,.29f,.32f,c.seam(),2);
    c.plate(.78f,.11f,.89f,.29f); c.line(.82f,.29f,.70f,.38f,c.dark(),2);
    c.line(.14f,.09f,.12f,.12f,c.ink()); c.line(.86f,.09f,.85f,.11f,c.ink());
    engineBell(c,{{.32f,.08f,.94f},{.45f,.13f,1.0f},{.56f,.19f,.98f},{.75f,.30f,.93f},{.92f,.43f,.89f},{.96f,.45f,.83f}});
    c.line(.31f,.56f,.69f,.56f,c.bright());
    c.line(.20f,.75f,.80f,.75f,c.seam());
    throat(c,.325f,.062f,.016f); exitRim(c,.44f,.965f,.029f);
}
void kolibri(const Canvas& c) {
    // Compact stubby assembly with a short bell and offset propellant bottle.
    c.plate(0,0,1,.14f); c.line(.04f,.11f,.96f,.11f,c.bright());
    c.plate(.11f,.17f,.25f,.34f);
    chamber(c,.34f,.15f,.32f,.25f);
    c.line(.39f,.28f,.43f,.37f,c.bright());
    c.line(.24f,.24f,.76f,.24f,c.bright(),2); c.line(.79f,.14f,.76f,.27f,c.dark(),2);
    engineBell(c,{{.39f,.10f,.91f},{.53f,.16f,.98f},{.77f,.28f,.90f},{.96f,.34f,.82f}});
    c.line(.23f,.77f,.77f,.77f,c.seam());
    throat(c,.395f,.072f,.020f); exitRim(c,.33f,.965f,.029f);
}
void engine(const Canvas& c,int id) {
    switch(id) {
        case 8: valiant(c); break;
        case 9: titan(c); break;
        case 2: hawk(c); break;
        case 10: frontier(c); break;
        case 11: kolibri(c); break;
        default: throw std::logic_error("Engine is missing its vector design");
    }
}
void nose(const Canvas& c) {
    // Pointed crown with a separate heat-resistant tip and attachment skirt.
    auto roof=[](float u){return .88f*std::pow(std::abs(2*u-1),.97f);};
    c.skin(0,1,roof,[](float){return 1.0f;});
    c.poly({{.5f,0},{.35f,roof(.35f)},{.65f,roof(.65f)}},tinted({110,124,135,255},c.tint));
    c.curve(0,1,roof,c.ink());
    c.line(0,.88f,0,1,c.ink()); c.line(1,.88f,1,1,c.ink()); c.line(0,1,1,1,c.ink());
    c.line(.35f,roof(.35f),.65f,roof(.65f),c.seam());
    c.line(.02f,.91f,.98f,.91f,c.seam()); c.line(.03f,.96f,.97f,.96f,c.bright());
    c.line(.28f,.71f,.38f,.40f,c.bright()); c.line(.72f,.71f,.62f,.40f,c.seam());
}
void sideNose(const Canvas& c) {
    // Sloped outer shell has a vertical mounting face for radial use.
    auto roof=[](float u){return .83f*std::pow(u,.82f);};
    c.skin(0,1,roof,[](float){return 1.0f;});
    c.poly({{0,0},{.27f,roof(.27f)},{0,roof(.27f)}},tinted({112,127,137,255},c.tint));
    c.curve(0,1,roof,c.ink()); c.line(0,0,0,1,c.ink());
    c.line(1,.83f,1,1,c.ink()); c.line(0,1,1,1,c.ink());
    c.line(0,roof(.27f),.27f,roof(.27f),c.seam());
    c.line(.04f,.90f,.96f,.90f,c.seam()); c.line(.04f,.96f,.96f,.96f,c.bright());
    c.line(.12f,.29f,.56f,.68f,c.bright());
}
void decoupler(const Canvas& c) {
    // Two full-width mating rims and a recessed circumferential split.
    c.band(0,1); c.frame(0,0,1,1,c.ink());
    c.line(.02f,.14f,.98f,.14f,c.bright());
    c.rect(.02f,.40f,.96f,.20f,c.black());
    c.line(.02f,.37f,.98f,.37f,c.seam()); c.line(.02f,.62f,.98f,.62f,c.bright());
    c.line(.02f,.84f,.98f,.84f,c.seam());
    for(float x:{.12f,.32f,.68f,.88f}) c.bolt(x,.25f);
}
void sideSeparator(const Canvas& c) {
    // Two opposing, individually mounted housings joined by two shear pins.
    c.plate(0,.08f,.38f,.92f); c.plate(.62f,.08f,1,.92f);
    c.rect(.38f,.24f,.24f,.10f,c.seam()); c.rect(.38f,.66f,.24f,.10f,c.seam());
    c.line(.38f,.29f,.62f,.29f,c.ink()); c.line(.38f,.71f,.62f,.71f,c.ink());
    c.line(.06f,.17f,.06f,.83f,c.bright()); c.line(.94f,.17f,.94f,.83f,c.seam());
    c.bolt(.21f,.28f); c.bolt(.21f,.72f); c.bolt(.79f,.28f); c.bolt(.79f,.72f);
}
void landingLeg(const Canvas& c) {
    // Bracket, articulated upper link, telescoping piston and wide foot pad.
    c.plate(0,.15f,.19f,.73f);
    c.poly({{.12f,.21f},{.24f,.20f},{.69f,.71f},{.57f,.75f}},tinted({117,139,154,255},c.tint));
    c.outline({{.12f,.21f},{.24f,.20f},{.69f,.71f},{.57f,.75f}},c.ink());
    c.poly({{.14f,.56f},{.21f,.51f},{.58f,.76f},{.53f,.81f}},c.dark());
    c.outline({{.14f,.56f},{.21f,.51f},{.58f,.76f},{.53f,.81f}},c.ink());
    c.line(.18f,.53f,.54f,.78f,c.bright());
    c.poly({{.57f,.70f},{.66f,.69f},{.76f,.87f},{.67f,.88f}},tinted({145,160,168,255},c.tint));
    c.outline({{.57f,.70f},{.66f,.69f},{.76f,.87f},{.67f,.88f}},c.ink());
    c.circle(.18f,.24f,.095f,c.ink()); c.circle(.18f,.24f,.050f,c.bright());
    c.circle(.62f,.73f,.075f,c.ink()); c.circle(.62f,.73f,.035f,c.bright());
    c.poly({{.52f,.87f},{.88f,.87f},{1,.96f},{.46f,.96f}},c.dark());
    c.outline({{.52f,.87f},{.88f,.87f},{1,.96f},{.46f,.96f}},c.ink());
    c.line(.52f,.88f,.88f,.88f,c.bright());
}
void rcs(const Canvas& c) {
    // Compact central manifold with three individually outlined outlets.
    c.plate(.18f,.27f,.56f,.73f);
    c.poly({{0,.39f},{.18f,.39f},{.18f,.61f},{0,.61f}},c.dark());
    c.outline({{0,.39f},{.18f,.39f},{.18f,.61f},{0,.61f}},c.ink());
    c.poly({{.24f,.27f},{.17f,.05f},{.55f,.05f},{.48f,.27f}},c.dark());
    c.outline({{.24f,.27f},{.17f,.05f},{.55f,.05f},{.48f,.27f}},c.ink());
    c.rect(.18f,.05f,.36f,.035f,c.black());
    c.poly({{.24f,.73f},{.17f,.95f},{.55f,.95f},{.48f,.73f}},c.dark());
    c.outline({{.24f,.73f},{.17f,.95f},{.55f,.95f},{.48f,.73f}},c.ink());
    c.rect(.18f,.915f,.36f,.035f,c.black());
    c.poly({{.56f,.40f},{.90f,.27f},{.90f,.73f},{.56f,.60f}},c.dark());
    c.outline({{.56f,.40f},{.90f,.27f},{.90f,.73f},{.56f,.60f}},c.ink());
    c.rect(.87f,.30f,.03f,.40f,c.black()); c.bolt(.37f,.50f);
}
void panel(const Canvas& c) {
    // Closed, thin ribbed structural plate, distinct from the open truss.
    c.band(0,1); c.frame(0,0,1,1,c.ink());
    c.line(.02f,.19f,.98f,.19f,c.bright()); c.line(.02f,.81f,.98f,.81f,c.seam());
    int ribs=c.w>=36 && c.h>=9?8:4;
    for(int i=1;i<ribs;++i) {
        float x=static_cast<float>(i)/ribs;
        c.line(x,.22f,x,.78f,c.seam());
        if(c.w>=36 && c.h>=9) c.line(x+.012f,.22f,x+.012f,.78f,c.bright());
    }
    c.line(.05f,.93f,.95f,.93f,c.seam());
}
void truss(const Canvas& c) {
    // Open triangulated lattice; no solid fill behind its braces.
    Color rail=metalAt(0,c.tint,c.material), brace=metalAt(c.material.highlight,c.tint,c.material);
    float beam=std::max(1.0f,std::min(c.w,c.h)*.065f);
    c.line(.04f,.08f,.96f,.08f,rail,beam*1.7f); c.line(.04f,.92f,.96f,.92f,rail,beam*1.7f);
    c.line(.04f,.08f,.04f,.92f,rail,beam*1.7f); c.line(.96f,.08f,.96f,.92f,rail,beam*1.7f);
    c.line(.04f,.08f,.27f,.92f,brace,beam); c.line(.27f,.92f,.50f,.08f,brace,beam);
    c.line(.50f,.08f,.73f,.92f,brace,beam); c.line(.73f,.92f,.96f,.08f,brace,beam);
    c.line(.04f,.08f,.96f,.08f,c.ink()); c.line(.04f,.92f,.96f,.92f,c.ink());
    c.line(.04f,.08f,.04f,.92f,c.ink()); c.line(.96f,.08f,.96f,.92f,c.ink());
    c.bolt(.04f,.08f); c.bolt(.50f,.08f); c.bolt(.96f,.08f);
    c.bolt(.27f,.92f); c.bolt(.73f,.92f);
}
void parachute(const Canvas& c) {
    // Packed canopy: rounded cloth cap, tension straps and tapered base.
    auto top=[](float u){ float d=2*u-1; return .08f+.34f*(1-std::sqrt(std::max(0.0f,1-d*d))); };
    auto bottom=[](float u){return 1.0f-.12f*std::abs(2*u-1);};
    c.skin(0,1,top,bottom); c.curve(0,1,top,c.ink()); c.curve(0,1,bottom,c.ink());
    c.line(0,.42f,0,.88f,c.ink()); c.line(1,.42f,1,.88f,c.ink());
    c.line(.03f,.70f,.97f,.70f,c.seam()); c.line(.03f,.77f,.97f,.77f,c.bright());
    for(float x:{.28f,.50f,.72f}) {
        c.line(x,top(x)+.035f,x,.68f,c.seam());
        c.line(x+.018f,top(x+.018f)+.035f,x+.018f,.68f,c.bright());
    }
    c.line(.10f,.87f,.90f,.87f,c.seam());
}
void heatShield(const Canvas& c) {
    // Metallic attachment lip over a shallow, dark ablative heat shield.
    auto top=[](float){return 0.0f;};
    auto bottom=[](float u){float d=2*u-1; return .66f+.34f*std::sqrt(std::max(0.0f,1-d*d));};
    c.skin(0,1,top,bottom); c.line(0,0,1,0,c.ink());
    c.line(0,0,0,.66f,c.ink()); c.line(1,0,1,.66f,c.ink());
    c.skin(0,1,[](float){return .36f;},bottom,{135,130,121,255});
    c.line(.02f,.20f,.98f,.20f,c.bright()); c.line(0,.36f,1,.36f,c.ink());
    c.curve(0,1,bottom,c.ink());
    for(float x:{.16f,.32f,.50f,.68f,.84f}) c.line(x,.39f,x,bottom(x)-.04f,c.seam());
    c.line(.08f,.60f,.92f,.60f,c.seam());
}
void dockingPort(const Canvas& c) {
    // Narrow capture ring, chamfered shoulders and broad mounting flange.
    c.band(.30f,1); c.frame(0,.30f,1,1,c.ink());
    c.poly({{.22f,.30f},{.27f,0},{.73f,0},{.78f,.30f}},c.dark());
    c.outline({{.22f,.30f},{.27f,0},{.73f,0},{.78f,.30f}},c.ink());
    c.rect(.31f,.06f,.38f,.11f,c.black());
    c.line(.24f,.26f,.76f,.26f,c.bright());
    c.line(.03f,.65f,.97f,.65f,c.seam()); c.line(.03f,.77f,.97f,.77f,c.bright());
    c.bolt(.13f,.50f); c.bolt(.87f,.50f);
}
void shape(const PartDef& d,float x,float y,float w,float h,Color tint,bool selected) {
    Canvas c{x,y,w,h,tint,materialFor(d.id)};
    // Exhaustive dispatch: no structural fallback is allowed to silently
    // render an unrelated part. Catalog variants have explicit parameters.
    switch(d.shape) {
        case PartShape::Capsule: capsule(c); break;
        case PartShape::Probe: probe(c); break;
        case PartShape::Tank: tank(c,d.id); break;
        case PartShape::Engine: engine(c,d.id); break;
        case PartShape::NoseCone: nose(c); break;
        case PartShape::Decoupler: decoupler(c); break;
        case PartShape::SideSeparator: sideSeparator(c); break;
        case PartShape::LandingLeg: landingLeg(c); break;
        case PartShape::Rcs: rcs(c); break;
        case PartShape::Panel: panel(c); break;
        case PartShape::Truss: truss(c); break;
        case PartShape::SideNose: sideNose(c); break;
        case PartShape::Parachute: parachute(c); break;
        case PartShape::HeatShield: heatShield(c); break;
        case PartShape::DockingPort: dockingPort(c); break;
    }
    if(selected) DrawRectangleLinesEx({x,y,w,h},1.0f,tinted(ui::cyan,tint));
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

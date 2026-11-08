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
Color metalAt(float u,Color tint) {
    constexpr Color edge{96,115,138,255},light{166,181,193,255},mid{138,155,176,255};
    if(u<.22f) return mix(edge,light,u/.22f,tint);
    if(u<.62f) return mix(light,mid,(u-.22f)/.40f,tint);
    return mix(mid,edge,(u-.62f)/.38f,tint);
}
// Mesh strips use RLGL's own white pixel, not the font/shape atlas or any
// previous part texture. QUADS keep the texture and primitive mode stable as
// Raylib switches between text, standard shapes and these custom silhouettes.
template<class Top,class Bottom>
void shell(float x,float y,float w,float h,float from,float to,Top top,Bottom bottom,Color tint,Color tone) {
    constexpr int slices=32;
    rlSetTexture(rlGetTextureIdDefault());
    rlBegin(RL_QUADS);
    auto vertex=[](float px,float py,Color c) {
        rlColor4ub(c.r,c.g,c.b,c.a); rlTexCoord2f(.5f,.5f); rlVertex2f(px,py);
    };
    for(int i=0;i<slices;++i) {
        float a=from+(to-from)*i/slices,b=from+(to-from)*(i+1)/slices;
        Color ca=tinted(metalAt(a,tint),tone), cb=tinted(metalAt(b,tint),tone);
        vertex(x+w*a,y+h*top(a),ca); vertex(x+w*a,y+h*bottom(a),ca);
        vertex(x+w*b,y+h*bottom(b),cb); vertex(x+w*b,y+h*top(b),cb);
    }
    rlEnd(); rlSetTexture(0);
}
// The bell uses the same continuous metallic cross-light as the other parts,
// with shared-vertex vertical bands for a subtle rolled-metal finish. Clamping
// each band to the curved silhouette keeps shading inside the bell.
template<class Top>
void bellShell(float x,float y,float w,float h,float from,float to,Top top,Color tint,Color tone) {
    constexpr int slices=32;
    constexpr float bands[]{0.0f,.40f,.58f,.76f,.90f,1.0f};
    constexpr float light[]{.85f,.88f,1.0f,.89f,.97f,.83f};
    auto vertex=[&](float u,float v,float brightness) {
        Color base=tinted(metalAt(u,tint),tone);
        auto shade=[brightness](unsigned char c) { return static_cast<unsigned char>(std::round(c*brightness)); };
        rlColor4ub(shade(base.r),shade(base.g),shade(base.b),base.a);
        rlTexCoord2f(.5f,.5f); rlVertex2f(x+w*u,y+h*v);
    };
    auto intensity=[&](int band,float v) {
        float t=std::clamp((v-bands[band])/(bands[band+1]-bands[band]),0.0f,1.0f);
        return light[band]+(light[band+1]-light[band])*t;
    };
    rlSetTexture(rlGetTextureIdDefault()); rlBegin(RL_QUADS);
    for(int i=0;i<slices;++i) {
        float a=from+(to-from)*i/slices, b=from+(to-from)*(i+1)/slices;
        for(int row=0;row<5;++row) {
            float a0=std::max(top(a),bands[row]), a1=std::max(top(a),bands[row+1]);
            float b0=std::max(top(b),bands[row]), b1=std::max(top(b),bands[row+1]);
            vertex(a,a0,intensity(row,a0)); vertex(a,a1,intensity(row,a1));
            vertex(b,b1,intensity(row,b1)); vertex(b,b0,intensity(row,b0));
        }
    }
    rlEnd(); rlSetTexture(0);
}
struct Canvas {
    float x,y,w,h;
    Color tint;
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
        constexpr float stops[]{0,.22f,.62f,1};
        for(int i=0;i<3;++i) {
            float a=stops[i],b=stops[i+1]; Color ca=metalAt(a,tint),cb=metalAt(b,tint);
            DrawRectangleGradientEx({x+w*a,y+h*y0,w*(b-a),h*(y1-y0)},ca,ca,cb,cb);
        }
    }
    template<class Top,class Bottom>
    void skin(float from,float to,Top top,Bottom bottom,Color tone={255,255,255,255}) const { shell(x,y,w,h,from,to,top,bottom,tint,tone); }
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
    void bolt(float u,float v) const {
        circle(u,v,.035f,ink()); circle(u,v,.016f,bright());
    }
};

void capsule(const Canvas& c) {
    auto roof=[](float u){return .80f*std::pow(std::abs(2*u-1),1.35f);};
    c.skin(0,1,roof,[](float){return 1.0f;}); c.curve(0,1,roof,c.ink());
    c.line(0,.80f,0,1,c.ink()); c.line(1,.80f,1,1,c.ink()); c.line(0,1,1,1,c.ink());
    c.line(.025f,.87f,.975f,.87f,c.seam()); c.line(.025f,.91f,.975f,.91f,c.bright());
    c.line(.20f,.69f,.33f,.37f,c.seam()); c.line(.80f,.69f,.67f,.37f,c.seam());
    // The pod skin remains uninterrupted: no porthole, inset box or glass.
    c.line(.04f,.94f,.96f,.94f,c.seam());
}
void probe(const Canvas& c) {
    c.poly({{0,.18f},{.12f,0},{.88f,0},{1,.18f},{1,.82f},{.88f,1},{.12f,1},{0,.82f}},tinted({112,134,153,255},c.tint));
    c.rect(.12f,.08f,.76f,.84f,tinted({145,162,178,255},c.tint));
    c.frame(.12f,.08f,.88f,.92f,c.ink());
    c.rect(.26f,.24f,.48f,.52f,c.dark()); c.frame(.26f,.24f,.74f,.76f,c.ink());
    c.line(.31f,.35f,.69f,.35f,c.bright()); c.line(.31f,.65f,.69f,.65f,c.seam());
    c.line(.15f,.14f,.22f,.14f,c.bright()); c.line(.78f,.86f,.85f,.86f,c.seam());
    c.circle(.5f,.50f,.08f,tinted({70,143,180,255},c.tint));
    c.line(0,.18f,.12f,0,c.ink()); c.line(.88f,0,1,.18f,c.ink());
    c.line(0,.82f,.12f,1,c.ink()); c.line(.88f,1,1,.82f,c.ink());
    c.line(.12f,0,.88f,0,c.ink()); c.line(.12f,1,.88f,1,c.ink());
    c.line(0,.18f,0,.82f,c.ink()); c.line(1,.18f,1,.82f,c.ink());
}
void tank(const Canvas& c,int sections) {
    c.band(0,1); c.frame(0,0,1,1,c.ink());
    c.line(.02f,.07f,.98f,.07f,c.bright()); c.line(.02f,.93f,.98f,.93f,c.seam());
    for(int i=1;i<sections;++i) {
        float y=static_cast<float>(i)/sections;
        c.line(.02f,y-.012f,.98f,y-.012f,c.seam()); c.line(.02f,y+.012f,.98f,y+.012f,c.bright());
    }
    c.line(.12f,.10f,.12f,.90f,c.bright()); c.line(.88f,.10f,.88f,.90f,c.seam());
}
struct EngineStyle { float throat,rim,start,power,flange; int ribs,bolts; Color bellTone; };
EngineStyle engineStyle(int id) {
    // Different bell proportions and hardware are intentional, including when
    // two engines have the same catalog bounds.
    switch(id) {
        case 8: return {.14f,.40f,.38f,.78f,.15f,2,2,{199,202,197,255}}; // Valiant
        case 9: return {.21f,.47f,.34f,.55f,.19f,4,4,{161,169,171,255}}; // Titan
        case 2: return {.19f,.46f,.40f,.60f,.18f,3,2,{181,186,185,255}}; // Hawk
        case 10:return {.11f,.43f,.32f,.90f,.14f,5,4,{156,165,173,255}}; // Frontier
        case 11:return {.12f,.36f,.41f,.72f,.16f,1,1,{204,198,187,255}}; // Kolibri
        default: throw std::logic_error("Engine is missing its vector design");
    }
}
void engine(const Canvas& c,int id) {
    const auto e=engineStyle(id);
    c.band(0,e.flange); c.frame(0,0,1,e.flange,c.ink());
    c.line(.04f,e.flange*.72f,.96f,e.flange*.72f,c.seam());
    for(int i=0;i<e.bolts;++i) c.bolt((i+.5f)/e.bolts,e.flange*.45f);
    c.rect(.5f-e.throat,e.flange,e.throat*2,e.start-e.flange,c.dark());
    c.frame(.5f-e.throat,e.flange,.5f+e.throat,e.start,c.ink());
    c.line(.5f-e.throat,e.start-.075f,.5f+e.throat,e.start-.075f,c.bright());
    c.line(.5f-e.throat,e.start-.045f,.5f+e.throat,e.start-.045f,c.seam());
    c.ellipse(.5f,e.start-.025f,e.throat*.67f,.022f,c.ink());
    c.ellipse(.5f,e.start-.025f,e.throat*.46f,.012f,c.black());
    if(id!=11) {
        c.circle(.5f-e.throat-.07f,e.flange+.08f,.055f,c.ink());
        c.circle(.5f+e.throat+.07f,e.flange+.08f,.055f,c.ink());
        c.circle(.5f-e.throat-.07f,e.flange+.08f,.028f,c.bright());
        c.circle(.5f+e.throat+.07f,e.flange+.08f,.028f,c.bright());
    }
    auto top=[=](float u) {
        float d=std::abs(u-.5f);
        return d<=e.throat?e.start:e.start+(1-e.start)*std::pow(std::clamp((d-e.throat)/(e.rim-e.throat),0.0f,1.0f),e.power);
    };
    bellShell(c.x,c.y,c.w,c.h,.5f-e.rim,.5f+e.rim,top,c.tint,e.bellTone);
    c.line(.5f-e.throat,e.start,.5f+e.throat,e.start,c.ink());
    c.curve(.5f-e.rim,.5f-e.throat,top,c.ink()); c.curve(.5f+e.throat,.5f+e.rim,top,c.ink());
    for(int i=1;i<=e.ribs;++i) {
        float t=static_cast<float>(i)/(e.ribs+1), y=e.start+(1-e.start)*t;
        float r=e.throat+(e.rim-e.throat)*std::pow(t,1/e.power);
        c.line(.5f-r+.01f,y,.5f+r-.01f,y,c.seam());
    }
    // Layered, inset exhaust aperture and metal lip. The opening stays within
    // the side-view bell silhouette, including at the narrowest upper edge.
    float mouth=e.rim*.83f;
    c.ellipse(.5f,.945f,mouth,.053f,c.ink());
    c.ellipse(.5f,.944f,mouth*.96f,.043f,tinted({135,147,153,255},c.tint));
    c.ellipse(.5f,.941f,mouth*.88f,.032f,c.black());
    c.ellipse(.5f,.936f,mouth*.68f,.020f,tinted({48,57,63,255},c.tint));
    c.ellipse(.5f,.934f,mouth*.36f,.011f,tinted({20,29,35,255},c.tint));
    c.line(.5f-e.rim,1,.5f+e.rim,1,c.ink());
}
void nose(const Canvas& c) {
    auto roof=[](float u){return .92f*std::pow(std::abs(2*u-1),.88f);};
    c.skin(0,1,roof,[](float){return 1.0f;}); c.curve(0,1,roof,c.ink());
    c.line(0,.92f,0,1,c.ink()); c.line(1,.92f,1,1,c.ink()); c.line(0,1,1,1,c.ink());
    c.line(.03f,.95f,.97f,.95f,c.seam()); c.line(.04f,.98f,.96f,.98f,c.bright());
    c.line(.25f,.76f,.40f,.36f,c.bright()); c.line(.75f,.76f,.60f,.36f,c.seam());
}
void sideNose(const Canvas& c) {
    auto roof=[](float u){return .82f*std::pow(u,.72f);};
    c.skin(0,1,roof,[](float){return 1.0f;}); c.curve(0,1,roof,c.ink());
    c.line(0,0,0,1,c.ink()); c.line(1,.82f,1,1,c.ink()); c.line(0,1,1,1,c.ink());
    c.line(.06f,.93f,.94f,.93f,c.seam()); c.line(.06f,.97f,.94f,.97f,c.bright());
    c.line(.14f,.25f,.55f,.67f,c.bright());
}
void decoupler(const Canvas& c) {
    c.band(0,1); c.frame(0,0,1,1,c.ink());
    c.rect(.02f,.43f,.96f,.14f,c.black());
    c.line(.02f,.32f,.98f,.32f,c.bright()); c.line(.02f,.70f,.98f,.70f,c.seam());
    c.line(.03f,.90f,.97f,.90f,c.bright());
    for(float x:{.15f,.35f,.65f,.85f}) c.bolt(x,.24f);
}
void sideSeparator(const Canvas& c) {
    c.band(0,1); c.frame(0,0,1,1,c.ink());
    c.rect(.43f,.03f,.14f,.94f,c.black());
    c.line(.32f,.04f,.32f,.96f,c.bright()); c.line(.68f,.04f,.68f,.96f,c.seam());
    c.line(.08f,.06f,.08f,.94f,c.bright()); c.line(.92f,.06f,.92f,.94f,c.seam());
    c.bolt(.21f,.22f); c.bolt(.79f,.22f); c.bolt(.21f,.78f); c.bolt(.79f,.78f);
}
void landingLeg(const Canvas& c) {
    c.rect(0,.22f,.20f,.46f,c.dark()); c.frame(0,.22f,.20f,.68f,c.ink());
    // Stowed articulated arm, secondary brace, hinge and broad footpad.
    c.poly({{.10f,.27f},{.27f,.27f},{.70f,.83f},{.52f,.83f}},tinted({122,142,158,255},c.tint));
    c.line(.10f,.27f,.52f,.83f,c.ink()); c.line(.27f,.27f,.70f,.83f,c.ink());
    c.poly({{.17f,.54f},{.22f,.52f},{.58f,.80f},{.53f,.82f}},c.dark());
    c.line(.18f,.56f,.52f,.83f,c.ink()); c.line(.20f,.53f,.58f,.80f,c.bright());
    c.circle(.19f,.30f,.095f,c.ink()); c.circle(.19f,.30f,.050f,c.bright());
    c.circle(.59f,.82f,.070f,c.ink()); c.circle(.59f,.82f,.035f,c.bright());
    c.poly({{.48f,.86f},{.86f,.86f},{1,.96f},{.46f,.96f}},c.dark());
    c.line(.48f,.86f,.86f,.86f,c.bright()); c.line(.46f,.96f,1,.96f,c.ink());
}
void rcs(const Canvas& c) {
    c.rect(0,.36f,.43f,.28f,c.dark()); c.frame(0,.36f,.43f,.64f,c.ink());
    c.rect(.18f,.26f,.38f,.48f,tinted({141,158,170,255},c.tint)); c.frame(.18f,.26f,.56f,.74f,c.ink());
    c.poly({{.56f,.40f},{.88f,.27f},{.88f,.73f},{.56f,.60f}},c.dark());
    c.line(.56f,.40f,.88f,.27f,c.ink()); c.line(.56f,.60f,.88f,.73f,c.ink());
    c.rect(.85f,.30f,.03f,.40f,c.black()); c.line(.88f,.27f,.88f,.73f,c.ink());
    c.poly({{.24f,.26f},{.16f,.06f},{.55f,.06f},{.48f,.26f}},c.dark());
    c.poly({{.24f,.74f},{.16f,.94f},{.55f,.94f},{.48f,.74f}},c.dark());
    c.rect(.17f,.06f,.37f,.035f,c.black()); c.rect(.17f,.905f,.37f,.035f,c.black());
    c.line(.16f,.06f,.55f,.06f,c.ink()); c.line(.16f,.94f,.55f,.94f,c.ink());
    c.bolt(.37f,.5f);
}
void panel(const Canvas& c) {
    c.band(0,1); c.frame(0,0,1,1,c.ink());
    c.line(.02f,.15f,.98f,.15f,c.bright()); c.line(.02f,.85f,.98f,.85f,c.seam());
    // Keep the corrugations readable rather than merging into a dark block
    // when the panel is shown as a small catalog icon or zoomed far out.
    int corrugations=c.w>=36 && c.h>=9?8:4;
    for(int i=1;i<corrugations;++i) {
        float x=static_cast<float>(i)/corrugations;
        c.line(x,.19f,x,.81f,c.seam());
        if(c.w>=36 && c.h>=9) c.line(x+.015f,.19f,x+.015f,.81f,c.bright());
    }
}
void truss(const Canvas& c) {
    // Open lattice: only the rails and braces are filled; the grid remains visible through it.
    float beam=std::max(1.0f,std::min(c.w,c.h)*.075f);
    c.line(.04f,.08f,.96f,.08f,c.dark(),beam*1.6f); c.line(.04f,.92f,.96f,.92f,c.dark(),beam*1.6f);
    c.line(.04f,.08f,.04f,.92f,c.dark(),beam*1.6f); c.line(.50f,.08f,.50f,.92f,c.dark(),beam*1.2f); c.line(.96f,.08f,.96f,.92f,c.dark(),beam*1.6f);
    c.line(.05f,.10f,.49f,.90f,c.bright(),beam); c.line(.05f,.90f,.49f,.10f,c.bright(),beam);
    c.line(.51f,.10f,.95f,.90f,c.bright(),beam); c.line(.51f,.90f,.95f,.10f,c.bright(),beam);
    c.line(.04f,.08f,.96f,.08f,c.ink()); c.line(.04f,.92f,.96f,.92f,c.ink());
    c.line(.04f,.08f,.04f,.92f,c.ink()); c.line(.96f,.08f,.96f,.92f,c.ink());
    c.bolt(.05f,.09f); c.bolt(.50f,.09f); c.bolt(.95f,.09f);
}
void parachute(const Canvas& c) {
    auto top=[](float u){ float d=2*u-1; return .13f+.40f*(1-std::sqrt(std::max(0.0f,1-d*d))); };
    c.skin(0,1,top,[](float){return 1.0f;}); c.curve(0,1,top,c.ink());
    c.line(0,.53f,0,1,c.ink()); c.line(1,.53f,1,1,c.ink()); c.line(0,1,1,1,c.ink());
    c.line(.03f,.77f,.97f,.77f,c.seam()); c.line(.03f,.84f,.97f,.84f,c.bright());
    c.line(.04f,.94f,.96f,.94f,c.seam());
    c.line(.31f,.22f,.31f,.75f,c.seam()); c.line(.69f,.22f,.69f,.75f,c.seam());
    c.line(.43f,.16f,.43f,.75f,c.bright()); c.line(.57f,.16f,.57f,.75f,c.bright());
}
void heatShield(const Canvas& c) {
    auto top=[](float){return 0.0f;};
    auto bottom=[](float u){ float d=2*u-1; return 1.0f-.32f*d*d; };
    c.skin(0,1,top,bottom); c.line(0,0,1,0,c.ink()); c.curve(0,1,bottom,c.ink());
    c.line(0,0,0,.68f,c.ink()); c.line(1,0,1,.68f,c.ink());
    c.rect(.04f,.23f,.92f,.28f,c.dark());
    for(int row=0;row<2;++row) for(int col=0;col<8;++col) {
        Color ceramic=(row+col)%2 ? Color{76,89,96,255} : Color{101,111,113,255};
        c.rect(.04f+col*.115f,.23f+row*.14f,.115f,.14f,tinted(ceramic,c.tint));
    }
    c.line(.04f,.23f,.96f,.23f,c.ink()); c.line(.04f,.51f,.96f,.51f,c.ink());
    // Ceramic waffle cells fit inside the blunt skirt, including on short tiles.
    for(int i=1;i<8;++i) { float x=.04f+i*.115f; c.line(x,.23f,x,.51f,c.seam()); }
    c.line(.04f,.37f,.96f,.37f,c.seam());
    c.line(.09f,.62f,.91f,.62f,c.seam()); c.line(.12f,.68f,.88f,.68f,c.bright());
}
void dockingPort(const Canvas& c) {
    c.band(.28f,1); c.frame(0,.28f,1,1,c.ink());
    c.rect(.22f,.05f,.56f,.23f,c.dark()); c.frame(.22f,.05f,.78f,.28f,c.ink());
    c.rect(.33f,.08f,.34f,.10f,c.black()); c.line(.03f,.72f,.97f,.72f,c.seam());
    c.line(.03f,.82f,.97f,.82f,c.bright());
    c.bolt(.13f,.5f); c.bolt(.87f,.5f);
}
void shape(const PartDef& d,float x,float y,float w,float h,Color tint,bool selected) {
    Canvas c{x,y,w,h,tint};
    // Exhaustive dispatch: no structural fallback is allowed to silently
    // render an unrelated part. Catalog variants have explicit parameters.
    switch(d.shape) {
        case PartShape::Capsule: capsule(c); break;
        case PartShape::Probe: probe(c); break;
        case PartShape::Tank: tank(c,std::max(1,d.height/2)); break;
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

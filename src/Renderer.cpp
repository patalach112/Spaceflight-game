#include "Renderer.hpp"
#include "UIComponents.hpp"
#include <rlgl.h>
#include <algorithm>
#include <array>
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
        {{62,77,94,255},{113,130,146,255},{89,106,123,255},.31f,.68f},
        {{73,87,100,255},{160,172,180,255},{124,138,149,255},.18f,.71f},
        {{65,82,99,255},{163,181,194,255},{116,139,158,255},.30f,.58f},
        {{108,111,113,255},{237,237,234,255},{184,188,188,255},.24f,.65f},
        {{105,110,112,255},{234,234,232,255},{180,185,185,255},.20f,.60f},
        {{101,106,111,255},{227,229,228,255},{173,179,182,255},.26f,.72f},
        {{65,79,95,255},{118,133,148,255},{92,109,125,255},.27f,.67f},
        {{57,70,85,255},{107,123,139,255},{82,98,114,255},.34f,.65f},
        {{66,81,98,255},{121,137,151,255},{96,113,129,255},.26f,.69f},
        {{70,82,96,255},{115,130,143,255},{96,111,125,255},.30f,.64f},
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
    if(id<0 || id>=98) throw std::logic_error("Part is missing its material profile");
    if(id<static_cast<int>(sizeof(profiles)/sizeof(profiles[0]))) return profiles[id];
    // Additional size variants retain a related but individually keyed finish.
    // Their geometry and panel layout are also selected by dimensions below.
    int variant=id-22;
    int tone=variant/4;
    return {{static_cast<unsigned char>(92+tone),static_cast<unsigned char>(105+tone),
             static_cast<unsigned char>(120+tone+variant%4),255},
            {211,220,224,255},{166,179,187,255},.25f,.68f};
}
// Five independently controlled transitions give metal a soft cylindrical
// falloff without the hard bands of a two-colour gradient. Adjacent quads
// share the exact same stop colours, including for translucent drag ghosts.
std::array<float,6> metalStops(const MaterialProfile& p) {
    return {0.0f,p.highlight*.42f,p.highlight,p.shadow,(p.shadow+1.0f)*.5f,1.0f};
}
Color metalAt(float u,Color tint,const MaterialProfile& p) {
    const auto stops=metalStops(p);
    const Color white{255,255,255,255};
    const std::array<Color,6> colors{p.edge,mix(p.edge,p.light,.58f,white),p.light,p.mid,
                                     mix(p.mid,p.edge,.53f,white),p.edge};
    u=std::clamp(u,0.0f,1.0f);
    for(std::size_t i=0;i+1<stops.size();++i)
        if(u<=stops[i+1]) return mix(colors[i],colors[i+1],(u-stops[i])/(stops[i+1]-stops[i]),tint);
    return tinted(p.edge,tint);
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
    Color ink() const { return tint.a<255?tint:tinted({26,38,52,255},tint); }
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
        const auto stops=metalStops(material);
        for(std::size_t i=0;i+1<stops.size();++i) {
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
        const auto stops=metalStops(material);
        for(std::size_t i=0;i+1<stops.size();++i) {
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
        circle(u,v,.024f,ink()); circle(u,v,.010f,bright());
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
void tank(const Canvas& c,const PartDef& d) {
    switch(d.id) {
        case 5: tank4x2(c); return; case 6: tank4x4(c); return;
        case 1: tank4x6(c); return; case 7: tank4x8(c); return;
        default: break;
    }
    tankBody(c);
    c.line(.015f,.035f,.985f,.035f,c.bright()); c.line(.015f,.965f,.985f,.965f,c.seam());
    int sections=d.height/2;
    for(int i=1;i<sections;++i) {
        float y=static_cast<float>(i)/sections;
        c.line(.015f,y,.985f,y,c.seam());
    }
    // Vertical skin joints are spaced in grid units instead of stretching a
    // four-wide texture over all of the wider tank sizes.
    for(int i=2;i<d.width;i+=2) {
        float x=static_cast<float>(i)/d.width;
        c.line(x,.06f,x,.94f,tinted({147,162,172,255},c.tint));
    }
}
// Cubic radial profiles are authored for each bell. The curve is evaluated in
// engine-local coordinates and tessellated finely only for rasterization; it
// is not a scaled copy of a generic cone. The mesh uses a single material and
// continuous vertex colours, so its seams remain invisible at high zoom.
struct BellProfile { float top, bottom, neck, control1, control2, lip; };
float bellRadius(const BellProfile& p,float t) {
    float u=1-t;
    return u*u*u*p.neck+3*u*u*t*p.control1+3*u*t*t*p.control2+t*t*t*p.lip;
}
void engineBell(const Canvas& c,const BellProfile& p) {
    constexpr int rows=48;
    const auto stops=metalStops(c.material);
    auto point=[&](float t,float across) {
        return c.p(.5f+(2*across-1)*bellRadius(p,t),p.top+(p.bottom-p.top)*t);
    };
    auto vertex=[&](float t,float across) {
        Color base=metalAt(across,c.tint,c.material);
        // A restrained axial falloff keeps the rim grounded without visible
        // rings or abrupt colour changes at the tessellation boundaries.
        float shade=1.0f-.035f*t;
        rlColor4ub(static_cast<unsigned char>(std::round(base.r*shade)),
                   static_cast<unsigned char>(std::round(base.g*shade)),
                   static_cast<unsigned char>(std::round(base.b*shade)),base.a);
        Vector2 v=point(t,across); rlTexCoord2f(.5f,.5f); rlVertex2f(v.x,v.y);
    };
    rlSetTexture(rlGetTextureIdDefault()); rlBegin(RL_QUADS);
    for(int row=0;row<rows;++row) {
        float a=static_cast<float>(row)/rows,b=static_cast<float>(row+1)/rows;
        for(std::size_t i=0;i+1<stops.size();++i) {
            float left=stops[i],right=stops[i+1];
            vertex(a,left); vertex(b,left); vertex(b,right); vertex(a,right);
        }
    }
    rlEnd(); rlSetTexture(0);
    for(int row=0;row<rows;++row) {
        float a=static_cast<float>(row)/rows,b=static_cast<float>(row+1)/rows;
        float y0=p.top+(p.bottom-p.top)*a,y1=p.top+(p.bottom-p.top)*b;
        c.line(.5f-bellRadius(p,a),y0,.5f-bellRadius(p,b),y1,c.ink());
        c.line(.5f+bellRadius(p,a),y0,.5f+bellRadius(p,b),y1,c.ink());
    }
}
// Narrow, restrained surface joints, following the local bell silhouette.
void bellJoint(const Canvas& c,const BellProfile& p,float t) {
    float r=bellRadius(p,t),y=p.top+(p.bottom-p.top)*t;
    c.line(.5f-r+.012f,y,.5f+r-.012f,y,tinted({67,82,97,255},c.tint));
}
void exitRim(const Canvas& c,float width,float y,float depth) {
    c.ellipse(.5f,y,width,depth,c.ink());
    c.ellipse(.5f,y-.002f,width*.965f,depth*.79f,tinted({135,147,153,255},c.tint));
    c.ellipse(.5f,y-.003f,width*.89f,depth*.57f,c.black());
    c.ellipse(.5f,y-.006f,width*.61f,depth*.26f,tinted({49,58,68,255},c.tint));
}
void throat(const Canvas& c,float y,float width,float depth) {
    c.ellipse(.5f,y,width,depth,c.ink());
    c.ellipse(.5f,y,width*.70f,depth*.59f,c.black());
}
// Shallow gimbal housing with a cylindrical highlight, dark seals and a
// single mechanical collar; not a painted-on window or generic ball.
void chamber(const Canvas& c,float x,float y,float w,float h) {
    MaterialProfile finish=c.material;
    auto brighten=[](unsigned char v,int d) { return static_cast<unsigned char>(std::min(255,int(v)+d)); };
    finish.edge={brighten(finish.edge.r,12),brighten(finish.edge.g,12),brighten(finish.edge.b,12),255};
    finish.light={brighten(finish.light.r,16),brighten(finish.light.g,16),brighten(finish.light.b,16),255};
    finish.mid={brighten(finish.mid.r,14),brighten(finish.mid.g,14),brighten(finish.mid.b,14),255};
    Canvas body{c.x+c.w*x,c.y+c.h*y,c.w*w,c.h*h,c.tint,finish};
    // Chamfered chamber in straight geometric segments. The breakpoints
    // coincide with the shell mesh so the one-pixel outline meets the fill.
    auto top=[](float u){return u<.1875f?.16f*(1-u/.1875f):u>.8125f?.16f*(u-.8125f)/.1875f:0.0f;};
    auto bottom=[](float u){return u<.1875f?.80f+.18f*u/.1875f:u>.8125f?.80f+.18f*(1-u)/.1875f:.98f;};
    body.skin(0,1,top,bottom);
    body.outline({{0,.16f},{.1875f,0},{.8125f,0},{1,.16f},
                  {1,.80f},{.8125f,.98f},{.1875f,.98f},{0,.80f}},body.ink());
    body.line(.04f,.35f,.96f,.35f,body.seam());
    body.line(.04f,.68f,.96f,.68f,body.seam());
}
// Narrow struts are filled polygons with one-pixel outlines, rather than
// scale-dependent thick strokes. Their design is stable at icon and VAB size.
void support(const Canvas& c,float x0,float y0,float x1,float y1,float thickness) {
    float half=.006f*thickness;
    c.poly({{x0-half,y0},{x0+half,y0},{x1+half,y1},{x1-half,y1}},tinted({93,111,127,255},c.tint));
    c.outline({{x0-half,y0},{x0+half,y0},{x1+half,y1},{x1-half,y1}},c.ink());
}
void valiant(const Canvas& c) {
    const BellProfile bell{.405f,.955f,.085f,.105f,.16f,.275f};
    c.plate(0,0,1,.09f); c.line(.03f,.06f,.97f,.06f,c.bright());
    c.bolt(.12f,.045f); c.bolt(.88f,.045f);
    // Two tall propellant-feed housings give this upper-stage engine its own
    // narrow, vertically articulated silhouette.
    c.plate(.19f,.105f,.29f,.31f); c.plate(.71f,.105f,.81f,.31f);
    support(c,.29f,.25f,.37f,.38f,1); support(c,.71f,.25f,.63f,.38f,1);
    chamber(c,.35f,.105f,.30f,.285f);
    c.plate(.33f,.215f,.67f,.26f);
    engineBell(c,bell); bellJoint(c,bell,.72f);
    throat(c,.408f,.068f,.016f); exitRim(c,.275f,.963f,.027f);
}
void titan(const Canvas& c) {
    const BellProfile bell{.365f,.955f,.14f,.23f,.37f,.445f};
    c.plate(0,0,1,.115f); c.line(.025f,.083f,.975f,.083f,c.bright());
    for(float x:{.10f,.29f,.71f,.90f}) c.bolt(x,.052f);
    support(c,.22f,.115f,.32f,.36f,2); support(c,.78f,.115f,.68f,.36f,2);
    chamber(c,.30f,.12f,.40f,.23f); c.plate(.26f,.205f,.74f,.25f);
    // Side gas-generator outlets surround a single broad main bell. They are
    // visually separate auxiliary nozzles, not extra gameplay thrust sources.
    c.plate(.065f,.15f,.19f,.285f); c.plate(.81f,.15f,.935f,.285f);
    c.poly({{.085f,.285f},{.17f,.285f},{.19f,.405f},{.065f,.405f}},c.dark());
    c.outline({{.085f,.285f},{.17f,.285f},{.19f,.405f},{.065f,.405f}},c.ink());
    c.line(.075f,.396f,.18f,.396f,c.black());
    c.poly({{.83f,.285f},{.915f,.285f},{.935f,.405f},{.81f,.405f}},c.dark());
    c.outline({{.83f,.285f},{.915f,.285f},{.935f,.405f},{.81f,.405f}},c.ink());
    c.line(.82f,.396f,.925f,.396f,c.black());
    engineBell(c,bell); bellJoint(c,bell,.28f); bellJoint(c,bell,.61f);
    throat(c,.37f,.105f,.020f); exitRim(c,.445f,.963f,.027f);
}
void hawk(const Canvas& c) {
    const BellProfile bell{.30f,.955f,.11f,.20f,.31f,.405f};
    c.plate(0,0,1,.105f); c.line(.025f,.072f,.975f,.072f,c.bright());
    c.bolt(.13f,.047f); c.bolt(.87f,.047f);
    support(c,.19f,.105f,.32f,.30f,2); support(c,.81f,.105f,.68f,.30f,2);
    chamber(c,.30f,.105f,.40f,.20f); c.plate(.28f,.18f,.72f,.225f);
    c.plate(.08f,.13f,.16f,.29f); support(c,.16f,.25f,.29f,.31f,1);
    engineBell(c,bell); bellJoint(c,bell,.30f); bellJoint(c,bell,.60f);
    throat(c,.305f,.082f,.019f); exitRim(c,.405f,.963f,.027f);
}
void frontier(const Canvas& c) {
    const BellProfile bell{.38f,.955f,.095f,.14f,.29f,.395f};
    c.plate(0,0,1,.085f); c.line(.025f,.058f,.975f,.058f,c.bright());
    c.bolt(.11f,.04f); c.bolt(.89f,.04f);
    support(c,.22f,.085f,.35f,.36f,2); support(c,.78f,.085f,.65f,.36f,2);
    chamber(c,.34f,.095f,.32f,.265f); c.plate(.32f,.17f,.68f,.215f);
    c.plate(.13f,.12f,.22f,.25f); support(c,.22f,.25f,.34f,.32f,1);
    c.plate(.78f,.12f,.87f,.25f); support(c,.78f,.25f,.66f,.32f,1);
    engineBell(c,bell); bellJoint(c,bell,.52f);
    throat(c,.385f,.073f,.016f); exitRim(c,.395f,.963f,.027f);
}
void kolibri(const Canvas& c) {
    const BellProfile bell{.34f,.955f,.095f,.15f,.24f,.32f};
    c.plate(0,0,1,.13f); c.line(.05f,.09f,.95f,.09f,c.bright());
    c.plate(.30f,.15f,.70f,.29f);
    c.poly({{.36f,.29f},{.64f,.29f},{.61f,.34f},{.39f,.34f}},c.dark());
    c.outline({{.36f,.29f},{.64f,.29f},{.61f,.34f},{.39f,.34f}},c.ink());
    engineBell(c,bell); bellJoint(c,bell,.68f);
    throat(c,.345f,.072f,.016f); exitRim(c,.32f,.963f,.027f);
}
void engine(const Canvas& c,int id) {
    switch(id) {
        case 8: valiant(c); break; case 9: titan(c); break; case 2: hawk(c); break;
        case 10: frontier(c); break; case 11: kolibri(c); break;
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
// Additional shapes use the same grid-local canvas and 1px slate outlines as
// the original parts. Their nodes and visual mating faces share the same edge.
void adapter(const Canvas& c,const PartDef& d) {
    float inset=1.0f/d.width;
    auto top=[&](float x) { return x<inset ? 1.0f-x/inset : x>1-inset ? (x-(1-inset))/inset : 0.0f; };
    // Split at both shoulders so the tessellation includes the exact top
    // corners, even when the width does not divide 32 evenly.
    auto bottom=[](float){return 1.0f;};
    c.skin(0,inset,top,bottom);
    c.skin(inset,1-inset,top,bottom);
    c.skin(1-inset,1,top,bottom);
    c.outline({{inset,0},{1-inset,0},{1,1},{0,1}},c.ink());
    c.line(inset+.02f,.08f,1-inset-.02f,.08f,c.bright());
    c.line(.04f,.88f,.96f,.88f,c.seam());
    for(int i=1;i<d.width/2-1;++i) {
        float x=inset+(1-2*inset)*i/(d.width/2-1);
        c.line(x,.20f,x,.78f,c.seam());
    }
}
void wheel(const Canvas& c,const PartDef& d) {
    float anchor=static_cast<float>(d.nodes.front().offset.y)/d.height;
    c.plate(0,anchor-.13f,.22f,anchor+.13f);
    c.circle(.52f,.5f,.46f,c.ink());
    c.circle(.52f,.5f,.43f,tinted({48,60,73,255},c.tint));
    c.circle(.52f,.5f,.30f,tinted({98,116,132,255},c.tint));
    c.circle(.52f,.5f,.25f,c.ink());
    c.circle(.52f,.5f,.21f,tinted({151,166,175,255},c.tint));
    c.circle(.52f,.5f,.075f,c.ink());
    int treads=d.width==2?12:16;
    constexpr float tau=6.28318530718f;
    for(int i=0;i<treads;++i) {
        float a=tau*i/treads,cs=std::cos(a),sn=std::sin(a);
        c.line(.52f+.34f*cs,.5f+.34f*sn,.52f+.42f*cs,.5f+.42f*sn,c.seam());
    }
    for(int i=0;i<4;++i) {
        float a=tau*i/4; c.circle(.52f+.15f*std::cos(a),.5f+.15f*std::sin(a),.022f,c.ink());
    }
}
void heavyLeg(const Canvas& c) {
    c.plate(0,.23f,.18f,.68f);
    c.poly({{.12f,.28f},{.25f,.25f},{.72f,.72f},{.61f,.79f}},tinted({119,138,153,255},c.tint));
    c.outline({{.12f,.28f},{.25f,.25f},{.72f,.72f},{.61f,.79f}},c.ink());
    c.poly({{.14f,.59f},{.23f,.55f},{.59f,.79f},{.53f,.84f}},c.dark());
    c.outline({{.14f,.59f},{.23f,.55f},{.59f,.79f},{.53f,.84f}},c.ink());
    c.line(.18f,.58f,.55f,.81f,c.bright());
    c.circle(.18f,.32f,.065f,c.ink()); c.circle(.18f,.32f,.032f,c.bright());
    c.circle(.64f,.75f,.055f,c.ink()); c.circle(.64f,.75f,.025f,c.bright());
    c.poly({{.52f,.85f},{.83f,.85f},{.98f,.96f},{.42f,.96f}},c.dark());
    c.outline({{.52f,.85f},{.83f,.85f},{.98f,.96f},{.42f,.96f}},c.ink());
    c.line(.52f,.87f,.83f,.87f,c.bright());
}
void ionEngine(const Canvas& c) {
    c.plate(0,0,1,.16f); c.plate(.20f,.16f,.80f,.34f);
    c.poly({{.27f,.34f},{.73f,.34f},{.85f,.86f},{.15f,.86f}},tinted({90,108,125,255},c.tint));
    c.outline({{.27f,.34f},{.73f,.34f},{.85f,.86f},{.15f,.86f}},c.ink());
    c.ellipse(.5f,.87f,.35f,.08f,c.ink());
    c.ellipse(.5f,.865f,.30f,.055f,tinted({105,128,144,255},c.tint));
    c.ellipse(.5f,.86f,.22f,.035f,c.black());
    c.line(.25f,.55f,.75f,.55f,c.seam()); c.line(.20f,.73f,.80f,.73f,c.seam());
    c.line(.30f,.22f,.70f,.22f,c.bright());
}
void fairing(const Canvas& c,const PartDef& d) {
    // Separate shell side with a full-width lower mating face and an open
    // recessed interior panel; symmetry reflects it for the opposite half.
    c.poly({{0,0},{.50f,0},{.94f,.12f},{1,.24f},{1,1},{0,1}},tinted({153,170,183,255},c.tint));
    c.outline({{0,0},{.50f,0},{.94f,.12f},{1,.24f},{1,1},{0,1}},c.ink());
    c.poly({{.14f,.13f},{.40f,.13f},{.77f,.25f},{.82f,.90f},{.14f,.90f}},tinted({86,104,122,255},c.tint));
    c.outline({{.14f,.13f},{.40f,.13f},{.77f,.25f},{.82f,.90f},{.14f,.90f}},c.ink());
    c.line(.06f,.04f,.48f,.04f,c.bright());
    for(int i=1;i<d.height/4;++i) {
        float y=.25f+.62f*i/(d.height/4);
        c.line(.16f,y,.78f,y,c.seam());
    }
}
void engineBase(const Canvas& c,const PartDef& d) {
    tankBody(c); c.line(.02f,.10f,.98f,.10f,c.bright()); c.line(.02f,.80f,.98f,.80f,c.seam());
    int fins=d.width==6?2:4;
    for(int i=0;i<fins;++i) {
        float x=(i+.5f)/fins, half=.28f/fins;
        c.poly({{x-half,.78f},{x+half,.78f},{x+half*1.4f,1},{x-half*1.4f,1}},c.dark());
        c.outline({{x-half,.78f},{x+half,.78f},{x+half*1.4f,1},{x-half*1.4f,1}},c.ink());
    }
    for(int i=1;i<d.width/2;++i) c.line(static_cast<float>(i)/(d.width/2),.15f,static_cast<float>(i)/(d.width/2),.72f,c.seam());
}
void solarPanel(const Canvas& c,const PartDef& d) {
    c.plate(0,.42f,.15f,.58f);
    c.rect(.14f,.03f,.82f,.94f,tinted({38,65,91,255},c.tint));
    c.frame(.14f,.03f,.96f,.97f,c.ink());
    c.line(.16f,.05f,.94f,.05f,c.bright());
    int columns=d.width,rows=d.height;
    for(int i=1;i<columns;++i) { float x=.14f+.82f*i/columns; c.line(x,.04f,x,.96f,c.seam()); }
    for(int i=1;i<rows;++i) { float y=.03f+.94f*i/rows; c.line(.15f,y,.95f,y,c.seam()); }
}
void fin(const Canvas& c,const PartDef& d) {
    float tip=d.width==2?.92f:.98f;
    c.poly({{0,.20f},{.19f,.28f},{tip,.84f},{tip,.96f},{0,.96f}},tinted({139,157,169,255},c.tint));
    c.outline({{0,.20f},{.19f,.28f},{tip,.84f},{tip,.96f},{0,.96f}},c.ink());
    c.line(.10f,.36f,.10f,.91f,c.bright()); c.line(.13f,.90f,tip-.04f,.90f,c.seam());
    if(d.width>2) c.line(.20f,.41f,.74f,.82f,c.seam());
}
void hollowShield(const Canvas& c,const PartDef& d) {
    c.band(0,.22f); c.frame(0,0,1,.22f,c.ink());
    // Each half is convex; drawing as separate trapezoids leaves the center open.
    c.poly({{0,.22f},{.12f,.22f},{.20f,.76f},{.10f,1}},c.dark());
    c.poly({{.88f,.22f},{1,.22f},{.90f,1},{.80f,.76f}},c.dark());
    c.rect(.10f,.76f,.80f,.24f,c.dark());
    c.outline({{0,.22f},{.12f,.22f},{.20f,.76f},{.80f,.76f},{.88f,.22f},{1,.22f},{.90f,1},{.10f,1}},c.ink());
    for(int i=1;i<d.width/2;++i) c.line(static_cast<float>(i)/(d.width/2),.83f,static_cast<float>(i)/(d.width/2),.96f,c.seam());
}
void sideChute(const Canvas& c) {
    c.plate(0,.25f,.30f,.75f);
    c.poly({{.30f,.18f},{.64f,.18f},{.91f,.34f},{.91f,.66f},{.64f,.82f},{.30f,.82f}},tinted({152,169,181,255},c.tint));
    c.outline({{.30f,.18f},{.64f,.18f},{.91f,.34f},{.91f,.66f},{.64f,.82f},{.30f,.82f}},c.ink());
    c.line(.47f,.20f,.47f,.80f,c.seam()); c.line(.65f,.20f,.65f,.80f,c.seam());
    c.line(.33f,.27f,.84f,.27f,c.bright());
}
void stowedLeg(const Canvas& c,const PartDef& d) {
    c.plate(0,.23f,.18f,.68f);
    c.poly({{.12f,.20f},{.31f,.20f},{.44f,.72f},{.28f,.76f}},tinted({119,138,153,255},c.tint));
    c.outline({{.12f,.20f},{.31f,.20f},{.44f,.72f},{.28f,.76f}},c.ink());
    c.line(.19f,.30f,.34f,.69f,c.bright());
    c.circle(.18f,.30f,.06f,c.ink()); c.circle(.18f,.30f,.03f,c.bright());
    c.poly({{.24f,.73f},{.43f,.73f},{.52f,.82f},{.22f,.82f}},c.dark());
    c.outline({{.24f,.73f},{.43f,.73f},{.52f,.82f},{.22f,.82f}},c.ink());
    if(d.shape==PartShape::HeavyLeg) c.line(.24f,.37f,.37f,.67f,c.seam());
}
void stowedSolar(const Canvas& c,const PartDef& d) {
    c.plate(0,.42f,.16f,.58f);
    c.plate(.14f,.19f,.30f,.81f);
    c.rect(.17f,.23f,.10f,.54f,tinted({38,65,91,255},c.tint));
    c.frame(.17f,.23f,.27f,.77f,c.ink());
    for(int i=1;i<d.height;++i) { float y=.23f+.54f*i/d.height; c.line(.18f,y,.26f,y,c.seam()); }
}
void cargoFairing(const Canvas& c,const PartDef& d) {
    // Separate shell walls with a hollow centre. Collision remains the part's
    // rectangular construction footprint, as for other catalog pieces.
    c.plate(0,0,.15f,1); c.plate(.85f,0,1,1);
    c.line(0,0,1,0,c.ink()); c.line(0,1,1,1,c.ink());
    c.line(.03f,.05f,.12f,.05f,c.bright()); c.line(.88f,.05f,.97f,.05f,c.bright());
    for(int i=1;i<d.height;++i) {
        float y=static_cast<float>(i)/d.height;
        c.line(.02f,y,.13f,y,c.seam()); c.line(.87f,y,.98f,y,c.seam());
    }
    for(int i=1;i<d.width/2;++i) {
        float x=.15f+.70f*i/(d.width/2);
        c.line(x,.01f,x,.07f,c.seam());
    }
}
void shape(const PartDef& d,float x,float y,float w,float h,Color tint,bool selected,bool deployed) {
    Canvas c{x,y,w,h,tint,materialFor(d.id)};
    // Exhaustive dispatch: no structural fallback is allowed to silently
    // render an unrelated part. Catalog variants have explicit parameters.
    switch(d.shape) {
        case PartShape::Capsule: capsule(c); break;
        case PartShape::Probe: probe(c); break;
        case PartShape::Tank: tank(c,d); break;
        case PartShape::Engine: engine(c,d.id); break;
        case PartShape::NoseCone:
            nose(c);
            if(d.id>=22) for(int i=1;i<d.width/2;++i) { float x=static_cast<float>(i)/(d.width/2); c.line(x,.94f,x,.99f,c.seam()); }
            break;
        case PartShape::Decoupler:
            decoupler(c);
            if(d.id>=22) for(int i=1;i<d.width/2;++i) c.bolt(static_cast<float>(i)/(d.width/2),.75f);
            break;
        case PartShape::SideSeparator: sideSeparator(c); break;
        case PartShape::LandingLeg: if(deployed) landingLeg(c); else stowedLeg(c,d); break;
        case PartShape::Rcs: rcs(c); break;
        case PartShape::Panel: panel(c); break;
        case PartShape::Truss:
            truss(c);
            if(d.id>=22) for(int i=1;i<d.width/4;++i) c.line(static_cast<float>(i)/(d.width/4),.12f,static_cast<float>(i)/(d.width/4),.88f,c.seam());
            break;
        case PartShape::SideNose: sideNose(c); break;
        case PartShape::Parachute: parachute(c); break;
        case PartShape::HeatShield:
            heatShield(c);
            if(d.id>=22) for(int i=1;i<d.width/2;++i) c.bolt(static_cast<float>(i)/(d.width/2),.22f);
            break;
        case PartShape::DockingPort:
            dockingPort(c);
            if(d.id>=22) for(int i=1;i<d.width/2;++i) c.bolt(static_cast<float>(i)/(d.width/2),.51f);
            break;
        case PartShape::Adapter: adapter(c,d); break;
        case PartShape::Wheel: wheel(c,d); break;
        case PartShape::HeavyLeg: if(deployed) heavyLeg(c); else stowedLeg(c,d); break;
        case PartShape::IonEngine: ionEngine(c); break;
        case PartShape::Fairing: fairing(c,d); break;
        case PartShape::EngineBase: engineBase(c,d); break;
        case PartShape::SolarPanel: if(deployed) solarPanel(c,d); else stowedSolar(c,d); break;
        case PartShape::Fin: fin(c,d); break;
        case PartShape::HollowShield: hollowShield(c,d); break;
        case PartShape::SideChute: sideChute(c); break;
        case PartShape::CargoFairing: cargoFairing(c,d); break;
    }
    if(selected) DrawRectangleLinesEx({x,y,w,h},1.0f,tinted(ui::cyan,tint));
}
}
Vector2 Renderer::toScreen(double gx,double gy,const Viewport& v) { return {v.originX+static_cast<float>(gx)*20*v.zoom,v.originY+static_cast<float>(gy)*20*v.zoom}; }
Vector2 Renderer::toGrid(Vector2 p,const Viewport& v) { return {(p.x-v.originX)/(20*v.zoom),(p.y-v.originY)/(20*v.zoom)}; }
void Renderer::workspace(const Viewport& v,Rectangle area) {
    // The construction canvas has its own fixed palette. UI theme changes
    // must not silently change the background or the grid.
    constexpr Color canvas{40,52,66,255};      // #283442
    constexpr Color minorGrid{47,61,75,255};
    constexpr Color majorGrid{55,70,86,255};
    DrawRectangleRec(area,canvas);
    BeginScissorMode(static_cast<int>(area.x),static_cast<int>(area.y),static_cast<int>(area.width),static_cast<int>(area.height));
    float step=20*v.zoom; int stride=1; while(step*stride<12) stride*=2;
    Vector2 lo=toGrid({area.x,area.y},v),hi=toGrid({area.x+area.width,area.y+area.height},v);
    int sx=static_cast<int>(std::floor(lo.x/stride))*stride,ex=static_cast<int>(std::ceil(hi.x/stride))*stride;
    int sy=static_cast<int>(std::floor(lo.y/stride))*stride,ey=static_cast<int>(std::ceil(hi.y/stride))*stride;
    for(int x=sx;x<=ex;x+=stride) { float px=std::round(toScreen(x,0,v).x); DrawLineV({px,area.y},{px,area.y+area.height},x%4==0?majorGrid:minorGrid); }
    for(int y=sy;y<=ey;y+=stride) { float py=std::round(toScreen(0,y,v).y); DrawLineV({area.x,py},{area.x+area.width,py},y%4==0?majorGrid:minorGrid); }
    float axis=toScreen(0,0,v).x;
    DrawLineEx({axis,area.y},{axis,area.y+area.height},1.0f,Color{98,157,197,180});
    EndScissorMode();
}
void Renderer::part(const PartDef& d,double x,double y,int rotation,const Viewport& v,Color tint,bool selected,bool flipped,bool deployed) {
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
    shape(d,0,0,w,h,tint,selected,deployed);
    if(flipped) { rlDrawRenderBatchActive(); rlPopMatrix(); rlEnableBackfaceCulling(); }
    EndMode2D();
}
void Renderer::tilePart(const PartDef& d,Rectangle r,bool deployed) {
    float unit=std::min(r.width/(d.width+1.0f),r.height/(d.height+1.0f));
    float w=d.width*unit,h=d.height*unit; shape(d,r.x+(r.width-w)/2,r.y+(r.height-h)/2,w,h,ui::text,false,deployed);
}

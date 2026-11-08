#include "PartGeometry.hpp"
#include <algorithm>
#include <cmath>

namespace {
using Quad = std::array<GeometryPoint,4>;
Quad blend(const Quad& a,const Quad& b,double t) {
    Quad result{};
    for(std::size_t i=0;i<4;++i) result[i]={a[i].x+(b[i].x-a[i].x)*t,a[i].y+(b[i].y-a[i].y)*t};
    return result;
}
GeometryPoint transform(GeometryPoint p,const PartDef& d,GridPoint pos,int rotation,bool flipped) {
    p.x*=d.width; p.y*=d.height;
    if(flipped) p.x=d.width-p.x;
    double w=d.width,h=d.height;
    for(int i=0;i<normalizedRotation(rotation);++i) { p={h-p.y,p.x}; std::swap(w,h); }
    return {p.x+pos.x,p.y+pos.y};
}
double cross(GeometryPoint a,GeometryPoint b,GeometryPoint c) { return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x); }
Polygon hull(Polygon points) {
    std::sort(points.begin(),points.end(),[](auto a,auto b){return a.x<b.x || (a.x==b.x && a.y<b.y);});
    points.erase(std::unique(points.begin(),points.end(),[](auto a,auto b){return a.x==b.x && a.y==b.y;}),points.end());
    if(points.size()<3) return points;
    Polygon out(points.size()*2); std::size_t n=0;
    for(auto p:points) { while(n>=2 && cross(out[n-2],out[n-1],p)<=0) --n; out[n++]=p; }
    std::size_t lower=n;
    for(std::size_t i=points.size()-1;i-- > 0;) { auto p=points[i]; while(n>lower && cross(out[n-2],out[n-1],p)<=0) --n; out[n++]=p; }
    out.resize(n-1); return out;
}
bool separated(const Polygon& a,const Polygon& b) {
    constexpr double epsilon=1e-9;
    for(std::size_t i=0;i<a.size();++i) {
        auto p=a[i],q=a[(i+1)%a.size()]; double nx=-(q.y-p.y),ny=q.x-p.x;
        double amin=INFINITY,amax=-INFINITY,bmin=INFINITY,bmax=-INFINITY;
        for(auto v:a) { double d=v.x*nx+v.y*ny; amin=std::min(amin,d); amax=std::max(amax,d); }
        for(auto v:b) { double d=v.x*nx+v.y*ny; bmin=std::min(bmin,d); bmax=std::max(bmax,d); }
        if(amax<=bmin+epsilon || bmax<=amin+epsilon) return true;
    }
    return false;
}
}
std::vector<GeometryPiece> deploymentGeometry(const PartDef& d,double progress) {
    const double phase=std::isfinite(progress)?std::clamp(progress,0.0,1.0):0.0;
    const double t=phase*phase*(3-2*phase);
    std::vector<GeometryPiece> result;
    auto add=[&](GeometryMaterial m,Quad a,Quad b){result.push_back({blend(a,b,t),m});};
    if(d.shape==PartShape::SolarPanel) {
        Quad mount={{{0,.42},{.16,.42},{.16,.58},{0,.58}}};
        add(GeometryMaterial::Mount,mount,mount);
        add(GeometryMaterial::Solar,{{{.14,.19},{.30,.19},{.30,.81},{.14,.81}}},{{{.14,.03},{.96,.03},{.96,.97},{.14,.97}}});
    } else if(d.shape==PartShape::LandingLeg || d.shape==PartShape::HeavyLeg) {
        const bool heavy=d.shape==PartShape::HeavyLeg;
        Quad mount={{{0,.23},{.18,.23},{.18,.68},{0,.68}}};
        add(GeometryMaterial::Mount,mount,mount);
        add(GeometryMaterial::Link,{{{.14,.55},{.22,.51},{.38,.68},{.30,.72}}},{{{.14,.56},{.21,.51},{.58,.76},{.53,.81}}});
        add(GeometryMaterial::Arm,{{{.12,.20},{.31,.20},{.44,.72},{.28,.76}}},{{{.12,.21},{.24,.20},{.69,.71},{.57,.75}}});
        if(heavy) add(GeometryMaterial::Foot,{{{.22,.72},{.48,.72},{.56,.83},{.20,.83}}},{{{.52,.85},{.83,.85},{.98,.96},{.42,.96}}});
        else add(GeometryMaterial::Foot,{{{.24,.73},{.43,.73},{.52,.82},{.22,.82}}},{{{.52,.87},{.88,.87},{1,.96},{.46,.96}}});
    }
    return result;
}
std::vector<Polygon> partGeometry(const PartDef& d,GridPoint pos,int rotation,bool flipped,double progress) {
    std::vector<Polygon> result;
    if(supportsDeployment(d)) {
        for(const auto& piece:deploymentGeometry(d,progress)) {
            Polygon poly; for(auto p:piece.vertices) poly.push_back(transform(p,d,pos,rotation,flipped)); result.push_back(std::move(poly));
        }
    } else {
        Polygon poly; for(auto p:Quad{{{0,0},{1,0},{1,1},{0,1}}}) poly.push_back(transform(p,d,pos,rotation,flipped)); result.push_back(std::move(poly));
    }
    return result;
}
bool geometryContains(const std::vector<Polygon>& geometry,double x,double y) {
    GeometryPoint p{x,y};
    for(const auto& poly:geometry) {
        bool positive=false,negative=false;
        for(std::size_t i=0;i<poly.size();++i) { double c=cross(poly[i],poly[(i+1)%poly.size()],p); positive|=c>1e-9; negative|=c< -1e-9; }
        if(!(positive&&negative)) return true;
    }
    return false;
}
bool geometryOverlaps(const std::vector<Polygon>& a,const std::vector<Polygon>& b) {
    for(const auto& x:a) for(const auto& y:b) if(!separated(x,y) && !separated(y,x)) return true;
    return false;
}
std::vector<Polygon> sweptGeometry(const std::vector<Polygon>& from,const std::vector<Polygon>& to) {
    std::vector<Polygon> result;
    if(from.size()!=to.size()) {
        Polygon points;
        for(const auto& p:from) points.insert(points.end(),p.begin(),p.end());
        for(const auto& p:to) points.insert(points.end(),p.begin(),p.end());
        if(points.size()>=3) result.push_back(hull(std::move(points)));
        return result;
    }
    for(std::size_t i=0;i<from.size();++i) { auto points=from[i]; points.insert(points.end(),to[i].begin(),to[i].end()); result.push_back(hull(std::move(points))); }
    return result;
}

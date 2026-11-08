#include "VABSystem.hpp"
#include <cmath>
#include <algorithm>
#include <iterator>
namespace {
bool opposite(Direction a, Direction b) {
    return (a == Direction::Up && b == Direction::Down) || (a == Direction::Down && b == Direction::Up) ||
           (a == Direction::Left && b == Direction::Right) || (a == Direction::Right && b == Direction::Left);
}
bool same(GridPoint a, GridPoint b) { return a.x == b.x && a.y == b.y; }
bool overlap(GridPoint a, int aw, int ah, GridPoint b, int bw, int bh) {
    return a.x < b.x + bw && a.x + aw > b.x && a.y < b.y + bh && a.y + ah > b.y;
}
GridPoint mirror(GridPoint p, int width) { return {-p.x-width,p.y}; }
}
bool VABSystem::nodeOccupied(GridPoint point, Direction direction) const {
    for (const auto& part : parts_) {
        const auto& d=partById(part.defId);
        for (auto node : d.nodes) {
            node=rotatedNode(d,node,part.rotation,part.flipped);
            GridPoint p{part.position.x+node.offset.x,part.position.y+node.offset.y};
            if (same(p,point) && opposite(direction,node.direction)) return true;
        }
    }
    return false;
}
bool VABSystem::canPlace(int defId, GridPoint pos, int rotation, bool flipped, double progress, unsigned long long ignore) const {
    const auto& d=partById(defId);
    auto shape=partGeometry(d,pos,rotation,flipped,progress);
    for(const auto& part:parts_) {
        if(part.uid==ignore) continue;
        const auto& other=partById(part.defId);
        if(overlap(pos,partWidth(d,rotation),partHeight(d,rotation),part.position,partWidth(other,part.rotation),partHeight(other,part.rotation)) &&
           geometryOverlaps(shape,partGeometry(other,part.position,part.rotation,part.flipped,part.deployProgress))) return false;
    }
    return true;
}
Placement VABSystem::preview(int defId, double x, double y, bool symmetry, int rotation, bool flipped, double progress) const {
    const auto& d=partById(defId); rotation=normalizedRotation(rotation);
    int w=partWidth(d,rotation),h=partHeight(d,rotation);
    GridPoint desired{static_cast<int>(std::round(x-w/2.0)),static_cast<int>(std::round(y-h/2.0))};
    auto validAt=[&](GridPoint pos) {
        GridPoint mirrored=mirror(pos,w);
        return canPlace(defId,pos,rotation,flipped,progress) && (!symmetry || same(pos,mirrored) ||
            (canPlace(defId,mirrored,-rotation,!flipped,progress) &&
             !geometryOverlaps(partGeometry(d,pos,rotation,flipped,progress),partGeometry(d,mirrored,-rotation,!flipped,progress))));
    };
    bool gridValid=validAt(desired);
    Placement result{desired,{},false,gridValid,rotation,flipped};
    bool foundSnap=false; double best=2.25,bestInvalid=best;
    auto consider=[&](GridPoint target, AttachNode own, Direction other) {
        if (!opposite(own.direction,other)) return;
        GridPoint pos{target.x-own.offset.x,target.y-own.offset.y};
        double dx=pos.x-(x-w/2.0),dy=pos.y-(y-h/2.0),dist=dx*dx+dy*dy;
        if(dist>2.25) return;
        if(!validAt(pos)) {
            if(!foundSnap && !gridValid && dist<=bestInvalid) { bestInvalid=dist; result={pos,target,true,false,rotation,flipped}; }
            return;
        }
        if(dist<=best) { best=dist; foundSnap=true; result={pos,target,true,true,rotation,flipped}; }
    };
    for(auto own:d.nodes) {
        own=rotatedNode(d,own,rotation,flipped);
        for(const auto& part:parts_) {
            const auto& other=partById(part.defId);
            for(auto node:other.nodes) {
                node=rotatedNode(other,node,part.rotation,part.flipped);
                GridPoint point{part.position.x+node.offset.x,part.position.y+node.offset.y};
                if(!nodeOccupied(point,node.direction)) consider(point,own,node.direction);
            }
        }
    }
    return result;
}
bool VABSystem::place(int id,const Placement& p,bool symmetry,bool deployed,double progress) {
    int rotation=normalizedRotation(p.rotation);
    const auto& d=partById(id);
    deployed=deployed && supportsDeployment(d);
    if(!supportsDeployment(d)) progress=0;
    else if(!std::isfinite(progress) || progress<0) progress=deployed?1:0;
    progress=std::clamp(progress,0.0,1.0);
    if(!p.valid || !canPlace(id,p.position,rotation,p.flipped,progress)) return false;
    int w=partWidth(d,rotation);
    GridPoint mirrored=mirror(p.position,w);
    if(symmetry && !same(p.position,mirrored) &&
       (!canPlace(id,mirrored,-rotation,!p.flipped,progress) ||
        geometryOverlaps(partGeometry(d,p.position,rotation,p.flipped,progress),partGeometry(d,mirrored,-rotation,!p.flipped,progress)))) return false;
    parts_.push_back({id,p.position,nextUid_++,rotation,p.flipped,deployed,progress});
    if(symmetry && !same(p.position,mirrored)) parts_.push_back({id,mirrored,nextUid_++,normalizedRotation(-rotation),!p.flipped,deployed,progress});
    return true;
}
std::optional<PartInstance> VABSystem::removeAt(double x,double y) {
    for(auto it=parts_.rbegin();it!=parts_.rend();++it) {
        const auto& d=partById(it->defId);
        if(geometryContains(partGeometry(d,it->position,it->rotation,it->flipped,it->deployProgress),x,y)) {
            auto p=*it; parts_.erase(std::next(it).base()); return p;
        }
    }
    return std::nullopt;
}
bool VABSystem::removeById(unsigned long long uid) {
    for(auto it=parts_.begin();it!=parts_.end();++it) if(it->uid==uid) { parts_.erase(it); return true; }
    return false;
}
bool VABSystem::restore(const PartInstance& p) {
    if(!std::isfinite(p.deployProgress) || p.deployProgress<0 || p.deployProgress>1) return false;
    if(!canPlace(p.defId,p.position,p.rotation,p.flipped,p.deployProgress)) return false;
    parts_.push_back(p); return true;
}
const PartInstance* VABSystem::findById(unsigned long long uid) const {
    for(const auto& p:parts_) if(p.uid==uid) return &p;
    return nullptr;
}
bool VABSystem::toggleDeploymentById(unsigned long long uid) {
    for(auto& p:parts_) if(p.uid==uid && supportsDeployment(partById(p.defId))) { p.deployed=!p.deployed; p.deploymentBlocked=false; return true; }
    return false;
}
void VABSystem::updateAnimations(double seconds) {
    if(!std::isfinite(seconds) || seconds<=0) return;
    constexpr double duration=.65;
    for(auto& part:parts_) {
        const auto& def=partById(part.defId);
        if(!supportsDeployment(def)) continue;
        double goal=part.deployed?1.0:0.0;
        double next=part.deployProgress+std::clamp(goal-part.deployProgress,-seconds/duration,seconds/duration);
        if(next==part.deployProgress) { part.deploymentBlocked=false; continue; }
        auto start=partGeometry(def,part.position,part.rotation,part.flipped,part.deployProgress);
        auto clearTo=[&](double t) {
            auto swept=sweptGeometry(start,partGeometry(def,part.position,part.rotation,part.flipped,t));
            for(const auto& other:parts_) if(other.uid!=part.uid) {
                const auto& od=partById(other.defId);
                if(overlap(part.position,partWidth(def,part.rotation),partHeight(def,part.rotation),other.position,partWidth(od,other.rotation),partHeight(od,other.rotation)) &&
                   geometryOverlaps(swept,partGeometry(od,other.position,other.rotation,other.flipped,other.deployProgress))) return false;
            }
            return true;
        };
        if(clearTo(next)) { part.deployProgress=next; part.deploymentBlocked=false; }
        else {
            part.deploymentBlocked=true;
            // Stop at the obstacle rather than tunnelling or popping through it.
            double lo=0,hi=1;
            for(int i=0;i<20;++i) { double mid=(lo+hi)/2; if(clearTo(part.deployProgress+(next-part.deployProgress)*mid)) lo=mid; else hi=mid; }
            part.deployProgress+=(next-part.deployProgress)*lo;
        }
    }
}
bool VABSystem::rotateById(unsigned long long uid,int direction) {
    for(auto& p:parts_) if(p.uid==uid) {
        const auto& d=partById(p.defId);
        int rotation=normalizedRotation(p.rotation+direction);
        auto offsetX=[&](int r) { return static_cast<int>(std::floor((d.width-partWidth(d,r))/2.0)); };
        auto offsetY=[&](int r) { return static_cast<int>(std::floor((d.height-partHeight(d,r))/2.0)); };
        GridPoint pos{p.position.x-offsetX(p.rotation)+offsetX(rotation),p.position.y-offsetY(p.rotation)+offsetY(rotation)};
        if(!canPlace(p.defId,pos,rotation,p.flipped,p.deployProgress,uid)) return false;
        p.position=pos; p.rotation=rotation; return true;
    }
    return false;
}
Telemetry VABSystem::telemetry() const {
    Telemetry t; for(const auto& p:parts_) { const auto& d=partById(p.defId); t.mass+=d.dryMass+d.fuelCapacity; if(d.category==Category::Engines) t.thrust+=d.thrust; ++t.count; }
    if(t.mass>0) t.twr=t.thrust/(t.mass*9.80665);
    return t;
}
CenterOfMass VABSystem::centerOfMass() const {
    CenterOfMass result;
    for(const auto& p:parts_) { const auto& d=partById(p.defId); double m=d.dryMass+d.fuelCapacity;
        result.mass+=m; result.x+=(p.position.x+partWidth(d,p.rotation)/2.0)*m; result.y+=(p.position.y+partHeight(d,p.rotation)/2.0)*m;
    }
    if(result.mass>0) { result.x/=result.mass; result.y/=result.mass; }
    return result;
}

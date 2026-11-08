#include "VABSystem.hpp"
#include <cmath>
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
    if (point.y == 0 && direction == Direction::Down) return true;
    for (const auto& part : parts_) {
        const auto& d=partById(part.defId);
        for (auto node : d.nodes) {
            node=rotatedNode(d,node,part.rotation);
            GridPoint p{part.position.x+node.offset.x,part.position.y+node.offset.y};
            if (same(p,point) && opposite(direction,node.direction)) return true;
        }
    }
    return false;
}
bool VABSystem::canPlace(int defId, GridPoint pos, int rotation) const {
    const auto& d=partById(defId);
    int w=partWidth(d,rotation), h=partHeight(d,rotation);
    if (pos.y+h>0) return false;
    for (const auto& part : parts_) {
        const auto& other=partById(part.defId);
        if (overlap(pos,w,h,part.position,partWidth(other,part.rotation),partHeight(other,part.rotation))) return false;
    }
    return true;
}
Placement VABSystem::preview(int defId, double x, double y, bool symmetry, int rotation) const {
    const auto& d=partById(defId); rotation=normalizedRotation(rotation);
    int w=partWidth(d,rotation),h=partHeight(d,rotation);
    GridPoint desired{static_cast<int>(std::round(x-w/2.0)),static_cast<int>(std::round(y-h/2.0))};
    auto validAt=[&](GridPoint pos) {
        GridPoint mirrored=mirror(pos,w);
        return canPlace(defId,pos,rotation) && (!symmetry || same(pos,mirrored) ||
            (canPlace(defId,mirrored,-rotation) && !overlap(pos,w,h,mirrored,w,h)));
    };
    bool gridValid=validAt(desired);
    Placement result{desired,{},false,gridValid,rotation};
    bool foundSnap=false; double best=2.25,bestInvalid=best;
    auto consider=[&](GridPoint target, AttachNode own, Direction other) {
        if (!opposite(own.direction,other)) return;
        GridPoint pos{target.x-own.offset.x,target.y-own.offset.y};
        double dx=pos.x-(x-w/2.0),dy=pos.y-(y-h/2.0),dist=dx*dx+dy*dy;
        if(dist>2.25) return;
        if(!validAt(pos)) {
            if(!foundSnap && !gridValid && dist<=bestInvalid) { bestInvalid=dist; result={pos,target,true,false,rotation}; }
            return;
        }
        if(dist<=best) { best=dist; foundSnap=true; result={pos,target,true,true,rotation}; }
    };
    AttachNode base{{w/2,h},Direction::Down};
    consider({desired.x+base.offset.x,0},base,Direction::Up);
    for(auto own:d.nodes) {
        own=rotatedNode(d,own,rotation);
        for(const auto& part:parts_) {
            const auto& other=partById(part.defId);
            for(auto node:other.nodes) {
                node=rotatedNode(other,node,part.rotation);
                GridPoint point{part.position.x+node.offset.x,part.position.y+node.offset.y};
                if(!nodeOccupied(point,node.direction)) consider(point,own,node.direction);
            }
        }
    }
    return result;
}
bool VABSystem::place(int id,const Placement& p,bool symmetry) {
    int rotation=normalizedRotation(p.rotation);
    if(!p.valid || !canPlace(id,p.position,rotation)) return false;
    const auto& d=partById(id); int w=partWidth(d,rotation),h=partHeight(d,rotation);
    GridPoint mirrored=mirror(p.position,w);
    if(symmetry && !same(p.position,mirrored) && (!canPlace(id,mirrored,-rotation) || overlap(p.position,w,h,mirrored,w,h))) return false;
    parts_.push_back({id,p.position,nextUid_++,rotation});
    if(symmetry && !same(p.position,mirrored)) parts_.push_back({id,mirrored,nextUid_++,normalizedRotation(-rotation)});
    return true;
}
std::optional<PartInstance> VABSystem::removeAt(double x,double y) {
    for(auto it=parts_.rbegin();it!=parts_.rend();++it) {
        const auto& d=partById(it->defId);
        if(x>=it->position.x && x<it->position.x+partWidth(d,it->rotation) && y>=it->position.y && y<it->position.y+partHeight(d,it->rotation)) {
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
    if(!canPlace(p.defId,p.position,p.rotation)) return false;
    parts_.push_back(p); return true;
}
bool VABSystem::rotateById(unsigned long long uid,int direction) {
    for(auto& p:parts_) if(p.uid==uid) {
        auto old=p; const auto& d=partById(p.defId);
        int rotation=normalizedRotation(p.rotation+direction);
        // Map each orientation to a stable integer-grid anchor, avoiding drift
        // when even-by-odd parts are rotated repeatedly.
        auto offsetX=[&](int r) { return static_cast<int>(std::floor((d.width-partWidth(d,r))/2.0)); };
        auto offsetY=[&](int r) { return static_cast<int>(std::floor((d.height-partHeight(d,r))/2.0)); };
        GridPoint pos{p.position.x-offsetX(p.rotation)+offsetX(rotation),p.position.y-offsetY(p.rotation)+offsetY(rotation)};
        // Temporarily remove this part from collision checks, preserving its identity and order.
        p.position=pos; p.rotation=rotation;
        if(pos.y+partHeight(d,rotation)>0) { p=old; return false; }
        for(const auto& other:parts_) if(other.uid!=uid) {
            const auto& od=partById(other.defId);
            if(overlap(pos,partWidth(d,rotation),partHeight(d,rotation),other.position,partWidth(od,other.rotation),partHeight(od,other.rotation))) { p=old; return false; }
        }
        return true;
    }
    return false;
}
Telemetry VABSystem::telemetry() const {
    Telemetry t; for(const auto& p:parts_) { const auto& d=partById(p.defId); t.mass+=d.dryMass+d.fuelCapacity; t.thrust+=d.thrust; ++t.count; }
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

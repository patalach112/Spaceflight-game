#include "VABInteraction.hpp"
void VABInteraction::choosePart(int id) {
    partById(id); cancel(); selected_=0; dragId_=id; rotation_=0;
}
Placement VABInteraction::preview(double x,double y,bool symmetry) const {
    if(!active()) return {};
    return system_.preview(dragId_,x-offsetX_,y-offsetY_,symmetry && !moving(),rotation_);
}
double VABInteraction::ghostX(double x) const { return active()?x-offsetX_-partWidth(partById(dragId_),rotation_)/2.0:x; }
double VABInteraction::ghostY(double y) const { return active()?y-offsetY_-partHeight(partById(dragId_),rotation_)/2.0:y; }
Telemetry VABInteraction::telemetry() const {
    Telemetry t=system_.telemetry();
    if(moved_) { const auto& d=partById(moved_->defId); t.mass+=d.dryMass+d.fuelCapacity; t.thrust+=d.thrust; ++t.count; t.twr=t.mass>0?t.thrust/(t.mass*9.80665):0; }
    return t;
}
CenterOfMass VABInteraction::centerOfMass() const {
    auto c=system_.centerOfMass();
    if(moved_) { const auto& d=partById(moved_->defId); double m=d.dryMass+d.fuelCapacity;
        double total=c.mass+m;
        if(total>0) {
            c.x=(c.x*c.mass+(moved_->position.x+partWidth(d,moved_->rotation)/2.0)*m)/total;
            c.y=(c.y*c.mass+(moved_->position.y+partHeight(d,moved_->rotation)/2.0)*m)/total;
        }
        c.mass=total;
    }
    return c;
}
bool VABInteraction::finishDrag(double x,double y,bool inWorkspace,bool symmetry,bool dragged) {
    if(!active()) return false;
    if(moved_ && !dragged) { cancel(); return false; }
    auto p=preview(x,y,symmetry);
    bool placed=inWorkspace && system_.place(dragId_,p,symmetry && !moving());
    if(placed) { moved_.reset(); selected_=0; dragId_=-1; offsetX_=offsetY_=0; }
    else cancel();
    return placed;
}
void VABInteraction::beginMove(double x,double y) {
    if(active()) return;
    moved_=system_.removeAt(x,y);
    if(!moved_) { selected_=0; return; }
    dragId_=moved_->defId; rotation_=moved_->rotation; selected_=moved_->uid;
    const auto& d=partById(dragId_);
    offsetX_=x-(moved_->position.x+partWidth(d,rotation_)/2.0);
    offsetY_=y-(moved_->position.y+partHeight(d,rotation_)/2.0);
}
bool VABInteraction::rotate(int direction) {
    if(active()) {
        int steps=normalizedRotation(direction);
        for(int i=0;i<steps;++i) { double x=offsetX_; offsetX_=-offsetY_; offsetY_=x; }
        rotation_=normalizedRotation(rotation_+direction); return true;
    }
    return selected_ && system_.rotateById(selected_,direction);
}
void VABInteraction::cancel() {
    if(moved_) { system_.restore(*moved_); moved_.reset(); }
    dragId_=-1; rotation_=0; offsetX_=offsetY_=0;
}
void VABInteraction::clear() { cancel(); system_.clear(); selected_=0; }
void VABInteraction::eraseSelectedOrAt(double x,double y) {
    if(active()) return;
    if(selected_) system_.removeById(selected_); else system_.removeAt(x,y);
    selected_=0;
}

#include "VABSystem.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
void check(bool condition, const char* message) { if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); } }
void interactionTests();
int main() {
    VABSystem v;
    check(!v.preview(1,0,-4.6,false).snapped,"snap radius uses continuous cursor position");
    check(v.preview(1,0,-4.4,false).valid,"part within snap radius reaches floor");
    auto floor=v.preview(1,0,-3,false);
    check(floor.valid && floor.position.x==-2 && floor.position.y==-6,"tank snaps to floor");
    check(v.place(1,floor,false),"place tank");
    auto pod=v.preview(0,0,-8,false);
    check(pod.valid && pod.position.y==-10,"pod snaps to tank top");
    check(v.place(0,pod,false),"place pod");
    check(!v.preview(0,0,-8,false).valid,"occupied node and collision prevent duplicate");
    auto t=v.telemetry();
    check(t.count==2 && std::abs(t.mass-5.3)<1e-8 && t.thrust==0 && t.twr==0,"telemetry includes fuel mass");
    auto removed=v.removeAt(0,-8); check(removed && removed->defId==0,"hit removal");
    check(v.restore(*removed),"restore moved part");
    VABSystem symmetric;
    auto pair=symmetric.preview(1,5,-3,true);
    check(pair.valid && symmetric.place(1,pair,true),"mirror floor placement");
    check(symmetric.parts().size()==2 && symmetric.parts()[0].position.x==3 && symmetric.parts()[1].position.x==-7,"mirror across center axis");
    auto symmetricPod=symmetric.preview(0,5,-8,true);
    check(symmetricPod.valid && symmetric.place(0,symmetricPod,true),"mirror node placement");
    VABSystem center;
    auto centered=center.preview(1,0,-3,true); check(centered.valid && center.place(1,centered,true) && center.parts().size()==1,"centerline part is not duplicated");
    VABSystem atomic;
    auto pendingPair=atomic.preview(1,5,-3,true);
    auto obstruction=atomic.preview(1,-5,-3,false);
    check(obstruction.valid && atomic.place(1,obstruction,false),"add mirrored-side obstruction");
    check(!atomic.place(1,pendingPair,true) && atomic.parts().size()==1,"stale mirrored placement is atomic");
    VABSystem asymmetric;
    auto one=asymmetric.preview(1,5,-3,false); asymmetric.place(1,one,false);
    check(asymmetric.preview(0,5,-8,true).valid,"symmetry permits a clear grid position without support");
    VABSystem engine;
    auto engineBase=engine.preview(2,0,-1.5,false);
    check(engineBase.valid && engine.place(2,engineBase,false),"engine can be supported by the floor");
    auto tankAboveEngine=engine.preview(1,0,-6,false);
    check(tankAboveEngine.valid && engine.place(1,tankAboveEngine,false),"tank attaches above engine");
    auto powered=engine.telemetry();
    check(powered.thrust==120.0 && std::abs(powered.twr-120.0/(5.15*9.80665))<1e-8,"powered telemetry");
    VABSystem decoupler;
    auto base=decoupler.preview(3,0,-.5,false); check(base.valid && decoupler.place(3,base,false),"floor decoupler");
    // Engine can attach to the underside of a suspended tank, not through the floor.
    VABSystem stack;
    auto floorTank=stack.preview(1,0,-3,false); stack.place(1,floorTank,false);
    auto upper=stack.preview(1,0,-9,false); stack.place(1,upper,false);
    auto info=stack.telemetry(); check(info.count==2 && std::abs(info.mass-9.0)<1e-8,"stack mass");
    auto stale=upper; check(!stack.place(1,stale,false),"reject stale preview");
    VABSystem collisions;
    auto first=collisions.preview(1,0,-3,false); collisions.place(1,first,false);
    auto intersect=collisions.preview(1,2,-3,false);
    check(intersect.snapped && !intersect.valid, "floor snap rejects overlapping AABBs");
    auto uid=collisions.parts()[0].uid;
    check(collisions.removeById(uid) && !collisions.removeById(uid), "selection deletion is by stable id");
    check(collisions.parts().empty(), "deletion removes exactly one part");
    interactionTests();
    std::cout << "All construction and interaction tests passed.\n";
}

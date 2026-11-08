#include "VABInteraction.hpp"
#include "VABSystem.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
void check(bool condition, const char* message) { if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); } }
void interactionTests();
int main() {
    VABSystem v;
    check(!v.preview(1,0,-4.6,false).snapped,"snap radius uses continuous cursor position");
    check(v.preview(1,0,-4.4,false).valid,"grid position near reference line is valid");
    auto atAxis=v.preview(1,0,-3,false);
    check(atAxis.valid && atAxis.position.x==-2 && atAxis.position.y==-6,"tank can touch the reference line");
    check(v.place(1,atAxis,false),"place tank");
    auto pod=v.preview(0,0,-8,false);
    check(pod.valid && pod.position.y==-10,"pod snaps to tank top");
    check(v.place(0,pod,false),"place pod");
    check(!v.preview(0,0,-8,false).valid,"occupied node and collision prevent duplicate");
    auto t=v.telemetry();
    check(t.count==2 && std::abs(t.mass-19.0)<1e-8 && t.thrust==0 && t.twr==0,"telemetry includes fuel mass");
    auto removed=v.removeAt(0,-8); check(removed && removed->defId==0,"hit removal");
    check(v.restore(*removed),"restore moved part");
    VABSystem symmetric;
    auto pair=symmetric.preview(1,5,-3,true);
    check(pair.valid && symmetric.place(1,pair,true),"mirror placement");
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
    auto engineBase=engine.preview(2,0,-partById(2).height/2.0,false);
    check(engineBase.valid && engine.place(2,engineBase,false),"engine can touch the reference line");
    auto tankAboveEngine=engine.preview(1,0,-partById(2).height-3.0,false);
    check(tankAboveEngine.valid && engine.place(1,tankAboveEngine,false),"tank attaches above engine");
    auto powered=engine.telemetry();
    check(std::abs(powered.thrust-120.0*9.80665)<1e-8 && std::abs(powered.twr-120.0/18.5)<1e-8,"powered telemetry");
    VABSystem decoupler;
    auto base=decoupler.preview(3,0,-.5,false); check(base.valid && decoupler.place(3,base,false),"reference-line decoupler");
    // Adjacent tanks can still stack and contribute full mass.
    VABSystem stack;
    auto baseTank=stack.preview(1,0,-3,false); stack.place(1,baseTank,false);
    auto upper=stack.preview(1,0,-9,false); stack.place(1,upper,false);
    auto info=stack.telemetry(); check(info.count==2 && std::abs(info.mass-30.0)<1e-8,"stack mass");
    auto stale=upper; check(!stack.place(1,stale,false),"reject stale preview");
    VABSystem collisions;
    auto first=collisions.preview(1,0,-3,false); collisions.place(1,first,false);
    auto intersect=collisions.preview(1,2,-3,false);
    check(!intersect.valid, "grid placement rejects overlapping AABBs");
    auto uid=collisions.parts()[0].uid;
    check(collisions.removeById(uid) && !collisions.removeById(uid), "selection deletion is by stable id");
    check(collisions.parts().empty(), "deletion removes exactly one part");
    VABSystem flush;
    auto elevated=flush.preview(1,0,-12,false); check(flush.place(1,elevated,false),"place elevated tank");
    auto nose=flush.preview(0,0,-17,false); check(nose.valid && nose.snapped && flush.place(0,nose,false),"pod snaps to tank");
    check(flush.parts()[1].position.y + partHeight(partById(0),0) == flush.parts()[0].position.y,"pod bottom meets tank top exactly");
    auto motor=flush.preview(2,0,-9.0+partById(2).height/2.0,false); check(motor.valid && motor.snapped && flush.place(2,motor,false),"engine snaps to tank bottom");
    check(flush.parts()[0].position.y+partHeight(partById(1),0)==flush.parts()[2].position.y,"engine top meets tank bottom exactly");
    auto massCenter=flush.centerOfMass(); check(massCenter.mass>0 && std::abs(massCenter.x)<1e-8,"center of mass uses grid centers and mass");
    const auto& tank=partById(1); auto rotated=rotatedNode(tank,tank.nodes[0],1);
    check(partWidth(tank,1)==6 && partHeight(tank,1)==4 && rotated.offset.x==6 && rotated.offset.y==2 && rotated.direction==Direction::Right,"rotation transforms nodes and bounds");
    VABSystem rotatedBuild;
    auto horizontal=rotatedBuild.preview(1,8,-12,false,1); check(horizontal.valid && rotatedBuild.place(1,horizontal,false),"place rotated part");
    auto neighbor=rotatedBuild.preview(1,14,-12,false,1); check(neighbor.valid && neighbor.snapped && rotatedBuild.place(1,neighbor,false),"rotated side nodes snap");
    check(rotatedBuild.parts()[1].position.x==rotatedBuild.parts()[0].position.x+6,"rotated bounds touch flush");
    VABSystem rotatedPair;
    auto rp=rotatedPair.preview(2,10,-12,true,1); check(rp.valid && rotatedPair.place(2,rp,true),"rotated mirrored placement");
    check(rotatedPair.parts().size()==2 && rotatedPair.parts()[0].rotation==1 && rotatedPair.parts()[1].rotation==3,"mirrored orientation reflects correctly");
    VABSystem rotationBlocked;
    auto left=rotationBlocked.preview(1,0,-20,false); check(rotationBlocked.place(1,left,false),"place first obstacle part");
    auto right=rotationBlocked.preview(1,4,-20,false); check(rotationBlocked.place(1,right,false),"place adjacent obstacle part");
    auto before=rotationBlocked.parts()[0];
    check(!rotationBlocked.rotateById(before.uid,1),"rotation rejects overlap");
    check(rotationBlocked.parts()[0].position.x==before.position.x && rotationBlocked.parts()[0].rotation==before.rotation,"failed rotation rolls back");
    auto weighted=rotationBlocked.centerOfMass();
    check(std::abs(weighted.x-2.0)<1e-8 && std::abs(weighted.mass-30.0)<1e-8,"center of mass is mass weighted");
    VABSystem rotationCycle;
    auto e=rotationCycle.preview(2,20,-20,false); check(rotationCycle.place(2,e,false),"place engine for rotation cycle");
    auto initial=rotationCycle.parts()[0];
    for(int i=0;i<4;++i) check(rotationCycle.rotateById(initial.uid,1),"rotate engine quarter-turn");
    auto after=rotationCycle.parts()[0];
    check(after.rotation==0 && after.position.x==initial.position.x && after.position.y==initial.position.y,"four rotations do not drift on the grid");
    // The horizontal axis is only a reference: no bounds, artificial snap, or
    // occupied node is associated with y=0.
    VABSystem openGrid;
    auto below=openGrid.preview(1,10,10,false);
    check(below.valid && !below.snapped && below.position.y>0 && openGrid.place(1,below,false),"place entirely below reference line");
    auto across=openGrid.preview(1,0,0,false);
    check(across.valid && !across.snapped && across.position.y<0 && across.position.y+6>0 && openGrid.place(1,across,false),"place across reference line");
    check(openGrid.rotateById(openGrid.parts().back().uid,1),"rotate across reference line");
    VABSystem lineNodes;
    auto upperTank=lineNodes.preview(1,0,-3,false);
    check(lineNodes.place(1,upperTank,false),"place tank touching line");
    auto lowerEngine=lineNodes.preview(2,0,partById(2).height/2.0,false);
    check(lowerEngine.valid && lowerEngine.snapped && lowerEngine.position.y==0 && lineNodes.place(2,lowerEngine,false),"free downward node on reference line snaps below it");
    VABSystem radialSnap;
    auto radialTank=radialSnap.preview(1,0,0,false);
    check(radialSnap.place(1,radialTank,false),"place tank across reference line for radial attachments");
    auto legs=radialSnap.preview(14,3,0,true);
    check(legs.valid && legs.snapped && radialSnap.place(14,legs,true),"symmetrical legs snap to tank side nodes");
    const auto& legA=radialSnap.parts()[1]; const auto& legB=radialSnap.parts()[2];
    auto nodeA=rotatedNode(partById(14),partById(14).nodes[0],legA.rotation,legA.flipped);
    auto nodeB=rotatedNode(partById(14),partById(14).nodes[0],legB.rotation,legB.flipped);
    check(legA.position.x+nodeA.offset.x==2 && legB.position.x+nodeB.offset.x==-2 && nodeA.direction==Direction::Left && nodeB.direction==Direction::Right,"mirrored radial nodes face tank");
    VABSystem radialPair;
    auto radial=radialPair.preview(14,6,4,true,1);
    check(radial.valid && radialPair.place(14,radial,true),"place mirrored radial legs");
    auto leftNode=rotatedNode(partById(14),partById(14).nodes[0],radialPair.parts()[0].rotation,radialPair.parts()[0].flipped);
    auto rightNode=rotatedNode(partById(14),partById(14).nodes[0],radialPair.parts()[1].rotation,radialPair.parts()[1].flipped);
    check(leftNode.offset.x+rightNode.offset.x==partWidth(partById(14),1) && leftNode.offset.y==rightNode.offset.y,"mirrored rotated radial nodes reflect correctly");
    VABInteraction radialInput(radialPair);
    auto mirroredLeg=radialPair.parts()[1];
    radialInput.beginMove(mirroredLeg.position.x+1,mirroredLeg.position.y+1);
    check(radialInput.moving() && radialInput.flipped(),"moving a mirrored part preserves its reflection");
    radialInput.cancel();
    check(radialPair.parts().back().flipped,"cancel restores mirrored part");
    VABSystem capsuleStack;
    auto baseCapsule=capsuleStack.preview(0,0,0,false);
    check(capsuleStack.place(0,baseCapsule,false),"place capsule for top attachment");
    auto chute=capsuleStack.preview(19,0,-2.5,false);
    check(chute.valid && chute.snapped && capsuleStack.place(19,chute,false),"parachute attaches flush to capsule crown");
    check(capsuleStack.parts()[1].position.y+partById(19).height==capsuleStack.parts()[0].position.y,"capsule top and parachute bottom meet exactly");
    VABSystem control;
    auto rcs=control.preview(15,0,10,false);
    check(control.place(15,rcs,false) && control.telemetry().thrust==0,"RCS thrust is not included in main engine TWR");
    check(partCatalog().size()==83,"expanded catalog includes every implemented variant");
    for(auto category:{Category::Pods,Category::FuelTanks,Category::Engines,Category::Aerodynamics,Category::Fairings,Category::Structural,Category::Other}) {
        bool found=false; for(const auto& d:partCatalog()) found|=d.category==category;
        check(found,"each catalog tab contains parts");
    }
    check(partInCategory(partById(15),Category::Engines) && partInCategory(partById(15),Category::Other),"RCS is listed in engines and other");
    check(partInCategory(partById(69),Category::Engines) && partInCategory(partById(69),Category::Other),"ion engine is listed in engines and other");
    check(partInCategory(partById(4),Category::Pods) && partInCategory(partById(4),Category::Other),"probe is listed as both control and other");
    for(int w:{6,8,10,12}) for(int h:{2,4,6,8,16}) {
        const PartDef* tank=nullptr;
        for(const auto& d:partCatalog()) if(d.shape==PartShape::Tank && d.width==w && d.height==h) tank=&d;
        check(tank!=nullptr,"wide tank exists for each width and height");
        check(std::abs((tank->dryMass+tank->fuelCapacity)-w*h*.625)<1e-8 &&
              std::abs(tank->fuelCapacity/(tank->dryMass+tank->fuelCapacity)-.9)<1e-8,
              "wide tank mass and fuel follow the published progression");
        for(int w2:{4,6,8,10,12}) {
            const PartDef* mate=nullptr;
            for(const auto& d:partCatalog()) if(d.shape==PartShape::Tank && d.width==w2 && d.height==2) mate=&d;
            check(mate!=nullptr,"matching tank available");
            VABSystem build;
            auto base=build.preview(tank->id,0,0,false);
            check(build.place(tank->id,base,false),"place wide tank");
            double targetY=base.position.y+h+mate->height/2.0;
            auto snapped=build.preview(mate->id,0,targetY,false);
            check(snapped.valid && snapped.snapped && build.place(mate->id,snapped,false),"different tank widths snap and place");
            check(build.parts()[1].position.y==build.parts()[0].position.y+h,"different widths share an exact attachment plane");
        }
    }
    VABSystem ionBuild;
    auto ion=ionBuild.preview(69,0,20,false); check(ionBuild.place(69,ion,false),"place ion engine");
    check(std::abs(ionBuild.telemetry().thrust-1.5*9.80665)<1e-8,"ion engine contributes main thrust");
    for(int id:{66,67,68,74,75,76,77,82}) {
        VABSystem build; VABInteraction drag(build);
        drag.choosePart(id);
        check(drag.finishDrag(20,20,true,false,true),"new radial part can be dragged and released");
        check(build.parts().size()==1,"drag commits one new radial part");
    }

    check(partById(5).height<partById(6).height && partById(6).height<partById(1).height && partById(1).height<partById(7).height,"tank sizes increase");
    for(int id:{5,6,1,7}) { const auto& tankDef=partById(id); check(std::abs(tankDef.fuelCapacity/(tankDef.dryMass+tankDef.fuelCapacity)-.9)<1e-8,"tank mass is 90 percent fuel"); }
    for(int id:{8,9,2,10,11}) {
        const auto& d=partById(id);
        check(d.height>d.width && d.height*2==d.width*3,"engine bounds have a tall, consistent aspect ratio");
        check(partWidth(d,1)==d.height && partHeight(d,1)==d.width,"rotated engine occupancy uses corrected bounds");
        for(int r:{0,1}) {
            VABSystem bounds;
            auto firstEngine=bounds.preview(id,50,50,false,r);
            check(firstEngine.valid && bounds.place(id,firstEngine,false),"place engine for collision bounds");
            int w=partWidth(d,r),h=partHeight(d,r);
            double cx=firstEngine.position.x+w/2.0,cy=firstEngine.position.y+h/2.0;
            double dx=r ? w : 0,dy=r ? 0 : h;
            check(!bounds.preview(id,cx+dx-(r?1:0),cy+dy-(r?0:1),false,r).valid,
                  "engine collision covers its corrected full extent");
            check(bounds.preview(id,cx+dx,cy+dy,false,r).valid,
                  "engine bounds can touch without overlap");
        }
    }
    struct EngineReference { int id; const char* name; double mass, thrustTf; };
    for(const auto& expected: {EngineReference{8,"VALIANT ENGINE",2.0,40}, {9,"TITAN ENGINE",12.0,400},
                               {2,"HAWK ENGINE",3.5,120}, {10,"FRONTIER ENGINE",6.0,100},
                               {11,"KOLIBRI ENGINE",.5,15}}) {
        const auto& engine=partById(expected.id);
        check(engine.name==expected.name && engine.dryMass==expected.mass &&
              std::abs(engine.thrust/9.80665-expected.thrustTf)<1e-8,
              "engine names, masses and community thrust figures");
    }
    check(partById(12).width==4 && partById(12).height==4,"four-wide aerodynamic nose dimensions");
    struct MassReference { int id; double mass; };
    for(const auto& expected:{MassReference{12,.25}, {3,.4}, {13,.2}, {14,.5}, {15,.05},
                              {16,.2}, {18,.25}, {19,.4}, {20,.25}, {21,.19}})
        check(std::abs(partById(expected.id).dryMass-expected.mass)<1e-8,"community-reference utility mass");
    for(const auto& d:partCatalog()) {
        check(d.width>0 && d.height>0 && d.dryMass>=0 && d.fuelCapacity>=0 && d.thrust>=0,"valid catalog definition");
        check(&partById(d.id)==&d,"catalog ids are unique");
        for(auto node:d.nodes) check(node.offset.x>=0 && node.offset.x<=d.width && node.offset.y>=0 && node.offset.y<=d.height,"catalog node in part bounds");
        for(int r=0;r<4;++r) {
            VABSystem testBuild;
            auto p=testBuild.preview(d.id,20,20,false,r);
            check(p.valid && testBuild.place(d.id,p,false),"every catalog part is placeable below the line at every rotation");
        }
    }
    // Exercise every attachment node in every orientation and reflection.
    // Each candidate is dropped exactly onto a tank node and must meet it
    // without overlap or a grid-unit gap, on either side of the reference axis.
    for(const auto& d:partCatalog()) for(int r=0;r<4;++r) for(bool flip:{false,true}) {
        int w=partWidth(d,r),h=partHeight(d,r);
        for(auto original:d.nodes) {
            auto own=rotatedNode(d,original,r,flip);
            auto reflected=rotatedNode(d,original,-r,!flip);
            check(own.offset.x+reflected.offset.x==w && own.offset.y==reflected.offset.y,"every node reflects across the symmetry axis");
            auto mirrorDirection=[](Direction dir) {
                if(dir==Direction::Left) return Direction::Right;
                if(dir==Direction::Right) return Direction::Left;
                return dir;
            };
            check(reflected.direction==mirrorDirection(own.direction),"reflected node direction follows the artwork");
            for(auto target:partById(1).nodes) {
                auto opposite=[](Direction a,Direction b) {
                    return (a==Direction::Up && b==Direction::Down) || (a==Direction::Down && b==Direction::Up) ||
                           (a==Direction::Left && b==Direction::Right) || (a==Direction::Right && b==Direction::Left);
                };
                if(!opposite(own.direction,target.direction)) continue;
                VABSystem build;
                auto anchor=build.preview(1,0,0,false);
                check(build.place(1,anchor,false),"place node test anchor");
                GridPoint point{anchor.position.x+target.offset.x,anchor.position.y+target.offset.y};
                GridPoint expected{point.x-own.offset.x,point.y-own.offset.y};
                auto placement=build.preview(d.id,expected.x+w/2.0,expected.y+h/2.0,false,r,flip);
                check(placement.valid && placement.snapped && placement.position.x==expected.x && placement.position.y==expected.y,"exact node drops snap flush at every orientation");
                check(build.place(d.id,placement,false),"commit every exact node drop");
            }
        }
    }
    VABSystem distant;
    for(double y:{-100000.0,100000.0}) {
        auto p=distant.preview(1,100000.0,y,false);
        check(p.valid && distant.place(1,p,false),"canvas remains buildable far from the reference axes");
    }
    interactionTests();
    std::cout << "All construction and interaction tests passed.\n";
}

#include "PartGeometry.hpp"
#include "VABInteraction.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool ok,const char* message) { if(!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); } }
bool near(double a,double b) { return std::abs(a-b)<1e-8; }
GeometryPoint midpoint(const Polygon& p) { return {(p[0].x+p[2].x)/2,(p[0].y+p[2].y)/2}; }
}
void deploymentTests() {
    for(int id:{14,68,74,75}) {
        const auto& def=partById(id);
        require(supportsDeployment(def),"deployable is recognized");
        for(int i=0;i<=100;++i) {
            double progress=i/100.0;
            auto parts=deploymentGeometry(def,progress);
            require(!parts.empty(),"animated geometry has pieces");
            for(const auto& piece:parts) {
                double sign=0;
                for(int j=0;j<4;++j) {
                    auto a=piece.vertices[j],b=piece.vertices[(j+1)%4],c=piece.vertices[(j+2)%4];
                    require(a.x>=0 && a.x<=1 && a.y>=0 && a.y<=1,"geometry remains within its transform footprint");
                    double z=(b.x-a.x)*(c.y-b.y)-(b.y-a.y)*(c.x-b.x);
                    require(std::abs(z)>1e-10,"piece has no degenerate edges");
                    if(sign==0) sign=z;
                    require(sign*z>0,"collision pieces remain convex throughout the animation");
                }
            }
            for(int rotation=0;rotation<4;++rotation) for(bool flip:{false,true}) {
                auto world=partGeometry(def,{17,-23},rotation,flip,progress);
                require(world.size()==parts.size(),"render and hit geometry share components");
                for(std::size_t j=0;j<world.size();++j) for(int k=0;k<4;++k) {
                    auto local=parts[j].vertices[k]; double x=local.x*def.width,y=local.y*def.height,w=def.width,h=def.height;
                    if(flip) x=w-x;
                    for(int r=0;r<rotation;++r) { double nx=h-y; y=x; x=nx; std::swap(w,h); }
                    require(near(world[j][k].x,x+17) && near(world[j][k].y,y-23),"art and hitbox use identical rotation and reflection");
                }
                auto hit=midpoint(world[0]); require(geometryContains(world,hit.x,hit.y),"mount can be picked at every animation phase");
            }
        }
        auto start=deploymentGeometry(def,0),early=deploymentGeometry(def,.01),linear=deploymentGeometry(def,.1);
        double tiny=std::abs(early.back().vertices[2].x-start.back().vertices[2].x);
        double larger=std::abs(linear.back().vertices[2].x-start.back().vertices[2].x);
        require(tiny<larger*.02,"smoothstep eases motion at the start");
        for(int r=0;r<4;++r) for(bool flip:{false,true}) {
            VABSystem hitTest; Placement p{}; p.position={20,20}; p.rotation=r; p.flipped=flip; p.valid=true;
            require(hitTest.place(id,p,false),"place deployable for selection test");
            const auto& inst=hitTest.parts().front();
            auto world=partGeometry(def,inst.position,r,flip,0);
            auto hit=midpoint(world[0]);
            require(hitTest.removeAt(hit.x,hit.y).has_value(),"selection uses the transformed physical geometry");
        }
        auto stowed=partGeometry(def,{0,0},0,false,0);
        require(!geometryContains(stowed,def.width*.95,def.height*.1),"empty stowed footprint is not selectable");
        VABSystem emptyTest; Placement vacant{}; vacant.valid=true;
        require(emptyTest.place(id,vacant,false),"place part for empty-space selection test");
        require(!emptyTest.removeAt(def.width*.95,def.height*.1),"clicking blank footprint does not select the part");
        require(geometryContains(partGeometry(def,{0,0},0,false,1),def.width*.09,def.height*.5),"deployed artwork is selectable");
    }
    // Unoccupied space in the broad footprint can be used. Extending into a
    // solid tank must stop at contact, even when a single frame is very long.
    for(int id:{14,68,74,75}) for(int rotation=0;rotation<4;++rotation) for(bool flip:{false,true}) {
        VABSystem build;
        Placement panel{}; panel.position={20,20}; panel.rotation=rotation; panel.flipped=flip; panel.valid=true;
        require(build.place(id,panel,false,false),"place stowed deployable");
        auto original=build.parts().front();
        auto actual=partGeometry(partById(id),original.position,rotation,flip,0);
        auto deployed=partGeometry(partById(id),original.position,rotation,flip,1);
        bool obstaclePlaced=false;
        for(int x=16;x<26 && !obstaclePlaced;++x) for(int y=16;y<26 && !obstaclePlaced;++y) {
            Placement tank{}; tank.valid=true; tank.position={x,y};
            auto solid=partGeometry(partById(5),tank.position,0,false,0);
            if(!geometryOverlaps(actual,solid) && geometryOverlaps(deployed,solid) && build.place(5,tank,false)) obstaclePlaced=true;
        }
        require(obstaclePlaced,"place obstacle in extension path, outside stowed geometry");
        require(build.toggleDeploymentById(original.uid),"toggle deployment target");
        build.updateAnimations(10);
        auto moving=build.findById(original.uid);
        require(moving && moving->deployProgress<1 && moving->deploymentBlocked,"swept collision prevents tunnelling on long frames and reports blockage");
        require(!geometryOverlaps(partGeometry(partById(id),moving->position,rotation,flip,moving->deployProgress),
                                  partGeometry(partById(5),build.parts().back().position,0,false,0)),"animation stops before penetration");
        require(build.removeById(build.parts().back().uid),"remove obstacle");
        build.updateAnimations(1); require(near(build.findById(original.uid)->deployProgress,1) && !build.findById(original.uid)->deploymentBlocked,"animation resumes when obstruction is removed");
    }
    for(int id:{14,68,74,75}) {
        VABSystem build; VABInteraction input(build); input.choosePart(id);
        require(input.toggleDeployment(),"start ghost animation");
        require(near(input.deployProgress(),0),"toggle does not pop to target");
        input.updateAnimations(.13); double first=input.deployProgress();
        require(first>0 && first<1,"ghost advances smoothly");
        require(input.toggleDeployment(),"reverse ghost animation");
        require(near(input.deployProgress(),first),"reversal preserves exact current phase");
        input.updateAnimations(.065); require(input.deployProgress()<first && input.deployProgress()>0,"reversed ghost retracts smoothly");
        require(input.toggleDeployment(),"resume extension");
        auto before=input.deployProgress();
        require(input.finishDrag(12,20,true,true,true),"place animating mirrored pair");
        require(build.parts().size()==2 && near(build.parts()[0].deployProgress,before) && near(build.parts()[1].deployProgress,before),"placement preserves phase and mirrors it");
        auto selected=build.parts()[0];
        auto hit=midpoint(partGeometry(partById(id),selected.position,selected.rotation,selected.flipped,selected.deployProgress)[0]);
        input.beginMove(hit.x,hit.y); require(input.moving(),"pick animating piece");
        input.updateAnimations(.1); input.cancel();
        require(near(build.findById(selected.uid)->deployProgress,before),"cancel restores original animation phase");
        build.updateAnimations(1);
        require(near(build.findById(selected.uid)->deployProgress,1),"placed piece reaches target");
        require(build.toggleDeploymentById(selected.uid),"retract selected piece");
        build.updateAnimations(.2); auto reversed=build.findById(selected.uid)->deployProgress;
        require(reversed>0 && reversed<1,"placed piece retracts smoothly");
        require(build.toggleDeploymentById(selected.uid),"reverse placed piece");
        require(near(build.findById(selected.uid)->deployProgress,reversed),"placed reversal does not pop");
    }
}

#include "VABInteraction.hpp"
#include <cstdlib>
#include <iostream>
namespace {
void require(bool ok,const char* msg) { if(!ok) { std::cerr<<"FAIL: "<<msg<<'\n'; std::exit(1); } }
}
void interactionTests() {
    VABSystem v; VABInteraction input(v);
    input.choosePart(1);
    require(input.active() && v.parts().empty(),"pressing a palette tile starts a drag");
    auto floating=input.preview(8,-15,false);
    require(floating.valid && !floating.snapped,"clear grid positions work without a node");
    require(input.finishDrag(8,-15,true,false,true),"release places a palette ghost");
    require(!input.active() && v.parts().size()==1,"successful release clears drag state");
    input.choosePart(1);
    require(!input.finishDrag(8,-15,true,false,true) && !input.active(),"blocked palette drop discards ghost");
    input.choosePart(0);
    require(!input.finishDrag(0,0,false,false,true) && !input.active() && v.parts().size()==1,"release over sidebar discards palette part");
    input.choosePart(3);
    require(input.finishDrag(10,-1,true,true,true) && v.parts().size()==3,"symmetric drag places pair");
    input.beginMove(8,-15);
    require(input.moving() && input.telemetry().count==3,"dragging a placed part preserves telemetry");
    input.finishDrag(0,0,false,false,true);
    require(!input.active() && v.parts().size()==3,"release outside workspace restores moved part");
    input.beginMove(8,-15);
    input.finishDrag(10,-1,true,false,true);
    require(v.parts().size()==3 && !input.active(),"blocked move restores original part");
    input.beginMove(8,-15); input.finishDrag(20,-20,true,false,true);
    require(!input.active() && v.parts().size()==3,"valid move commits and releases cursor");
    input.beginMove(20,-20); input.finishDrag(20,-20,true,false,false);
    require(input.selected()!=0,"click without dragging selects part");
    require(input.rotate(1),"selected part rotates");
    require(v.parts().back().rotation==1,"rotation persists on selected part");
    input.eraseSelectedOrAt(0,0); require(v.parts().size()==2,"delete removes selection");
    input.choosePart(2); require(input.rotate(-1) && input.rotation()==3,"ghost rotation");
    input.cancel(); require(!input.active(),"cancel clears ghost");
    input.clear(); require(v.parts().empty() && input.selected()==0,"clear removes the build and selection");
}

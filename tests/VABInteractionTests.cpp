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
    input.choosePart(14);
    require(input.finishDrag(8,10,true,true,true),"symmetric palette drag places below the reference line");
    require(v.parts().size()==2 && v.parts()[1].flipped,"symmetric drag reflects radial part");
    input.beginMove(v.parts()[1].position.x+1,v.parts()[1].position.y+1);
    require(input.moving() && input.flipped(),"mirrored part retains reflection when picked up");
    require(input.finishDrag(-15,12,true,false,true),"move mirrored part below the reference line");
    require(v.parts().back().flipped,"moving mirrored part retains reflection");
    // Deployment belongs to each part instance, including reflected copies.
    VABSystem states; VABInteraction stateInput(states);
    stateInput.choosePart(74);
    require(stateInput.canToggleDeployment() && !stateInput.deployed(),"new solar panel starts stowed");
    require(stateInput.toggleDeployment() && stateInput.deployed(),"deployment can be previewed while dragging");
    require(stateInput.finishDrag(12,20,true,true,true),"place deployed mirrored solar panels");
    require(states.parts().size()==2 && states.parts()[0].deployed && states.parts()[1].deployed,"symmetry copies deployment state");
    auto panel=states.parts()[0];
    stateInput.beginMove(panel.position.x+1,panel.position.y+1);
    require(stateInput.moving() && stateInput.deployed(),"moving preserves deployment preview");
    require(stateInput.toggleDeployment() && !stateInput.deployed(),"moving can preview stow");
    stateInput.cancel();
    require(states.findById(panel.uid) && states.findById(panel.uid)->deployed,"cancel restores original deployment");
    stateInput.beginMove(panel.position.x+1,panel.position.y+1);
    require(stateInput.toggleDeployment(),"preview a state change before a rejected move");
    require(!stateInput.finishDrag(30,20,false,false,true),"out-of-workspace move is rejected");
    require(states.findById(panel.uid) && states.findById(panel.uid)->deployed,"rejected move restores original deployment");
    stateInput.beginMove(panel.position.x+1,panel.position.y+1);
    require(stateInput.toggleDeployment() && stateInput.rotate(1),"stow and rotate a moving panel");
    require(stateInput.finishDrag(30,20,true,false,true),"move a stowed panel");
    require(!states.parts().back().deployed && states.parts().back().rotation==1,"state survives a successful move and rotation");
    auto movedPanel=states.parts().back();
    stateInput.beginMove(movedPanel.position.x+1,movedPanel.position.y+1);
    require(!stateInput.finishDrag(movedPanel.position.x+1,movedPanel.position.y+1,true,false,false),"click selects panel without moving");
    require(stateInput.canToggleDeployment() && !stateInput.deployed() && stateInput.toggleDeployment(),"selected part toggles deployment");
    require(states.findById(movedPanel.uid)->deployed,"selected toggle changes the instance immediately");
    stateInput.choosePart(1);
    require(!stateInput.canToggleDeployment() && !stateInput.toggleDeployment(),"non-deployable parts reject toggles");
    stateInput.cancel();

}

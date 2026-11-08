#include "VABInteraction.hpp"
#include <cstdlib>
#include <iostream>
namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
}
void interactionTests() {
    VABSystem v;
    VABInteraction input(v);
    input.choosePart(1);
    require(input.active() && v.parts().empty(), "catalog click only picks up a ghost");
    auto floating = input.preview(8, -15, false);
    require(floating.valid && !floating.snapped, "clear grid positions are valid without a node");
    require(input.placeClick(8, -15, false), "subsequent click places a free grid ghost");
    require(!input.active() && v.parts().size() == 1, "successful placement releases the cursor");
    input.choosePart(1);
    require(!input.placeClick(8, -15, false) && input.active(), "collision keeps ghost for retry");
    require(input.placeClick(15, -15, false) && !input.active(), "retry at a clear location succeeds");
    input.choosePart(0);
    input.cancel();
    require(!input.active() && v.parts().size() == 2, "cancel releases a palette ghost without placing it");
    input.choosePart(3);
    require(input.placeClick(10, -1, true) && v.parts().size() == 4, "symmetry click places two parts atomically");
    input.beginMove(8, -15);
    require(input.moving() && v.parts().size() == 3, "existing part can be picked up");
    require(input.telemetry().count == 4, "telemetry includes the part being moved");
    input.cancel();
    require(!input.active() && v.parts().size() == 4, "cancel restores a moving part");
    input.beginMove(8, -15);
    input.endMove(15, -15, true);
    require(v.parts().size() == 4, "invalid move restores original part");
    input.beginMove(8, -15);
    input.endMove(20, -20, true);
    require(!input.active() && v.parts().size() == 4, "valid move commits and releases cursor");
    input.beginMove(20, -20);
    input.endMove(20, -20, false);
    require(input.selected() != 0, "click without dragging selects part");
    input.eraseSelectedOrAt(0, 0);
    require(v.parts().size() == 3, "delete removes selected part");
}

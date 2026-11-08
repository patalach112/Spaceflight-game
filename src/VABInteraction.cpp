#include "VABInteraction.hpp"
void VABInteraction::choosePart(int id) {
    partById(id); // Reject invalid catalog IDs before changing the interaction state.
    cancel();
    selected_ = 0;
    dragId_ = id;
}
Placement VABInteraction::preview(double x, double y, bool symmetry) const {
    if (!active()) return {};
    return system_.preview(dragId_, x - offsetX_, y - offsetY_, symmetry && !moving());
}
Telemetry VABInteraction::telemetry() const {
    Telemetry t = system_.telemetry();
    if (moved_) {
        const auto& def = partById(moved_->defId);
        t.mass += def.dryMass + def.fuelCapacity;
        t.thrust += def.thrust;
        ++t.count;
        t.twr = t.mass > 0 ? t.thrust / (t.mass * 9.80665) : 0;
    }
    return t;
}
bool VABInteraction::placeClick(double x, double y, bool symmetry) {
    if (!active() || moving()) return false;
    const auto placement = preview(x, y, symmetry);
    if (!system_.place(dragId_, placement, symmetry)) return false;
    dragId_ = -1;
    offsetX_ = offsetY_ = 0;
    return true;
}
void VABInteraction::beginMove(double x, double y) {
    if (active()) return;
    moved_ = system_.removeAt(x, y);
    if (!moved_) { selected_ = 0; return; }
    dragId_ = moved_->defId;
    selected_ = moved_->uid;
    const auto& d = partById(dragId_);
    offsetX_ = x - (moved_->position.x + d.width / 2.0);
    offsetY_ = y - (moved_->position.y + d.height / 2.0);
}
void VABInteraction::endMove(double x, double y, bool dragged) {
    if (!moved_) return;
    const auto placement = preview(x, y, false);
    if (dragged && system_.place(dragId_, placement, false)) {
        moved_.reset();
        selected_ = 0;
        dragId_ = -1;
        offsetX_ = offsetY_ = 0;
    } else cancel();
}
void VABInteraction::cancel() {
    if (moved_) {
        system_.restore(*moved_);
        moved_.reset();
    }
    dragId_ = -1;
    offsetX_ = offsetY_ = 0;
}
void VABInteraction::eraseSelectedOrAt(double x, double y) {
    if (active()) return;
    if (selected_) system_.removeById(selected_);
    else system_.removeAt(x, y);
    selected_ = 0;
}

#include "VABSystem.hpp"
#include <cmath>

namespace {
bool opposite(Direction a, Direction b) {
    return (a == Direction::Up && b == Direction::Down) || (a == Direction::Down && b == Direction::Up) ||
           (a == Direction::Left && b == Direction::Right) || (a == Direction::Right && b == Direction::Left);
}
bool same(GridPoint a, GridPoint b) { return a.x == b.x && a.y == b.y; }
bool overlap(GridPoint a, const PartDef& ad, GridPoint b, const PartDef& bd) {
    return a.x < b.x + bd.width && a.x + ad.width > b.x && a.y < b.y + bd.height && a.y + ad.height > b.y;
}
GridPoint mirror(GridPoint p, const PartDef& d) { return {-p.x - d.width, p.y}; }
}
bool VABSystem::nodeOccupied(GridPoint point, Direction direction) const {
    if (point.y == 0 && direction == Direction::Down) return true;
    for (const auto& part : parts_) for (const auto& node : partById(part.defId).nodes) {
        GridPoint p{part.position.x + node.offset.x, part.position.y + node.offset.y};
        if (same(p, point) && opposite(direction, node.direction)) return true;
    }
    return false;
}
bool VABSystem::canPlace(int defId, GridPoint pos) const {
    const auto& def = partById(defId);
    if (pos.y + def.height > 0) return false;
    for (const auto& part : parts_) if (overlap(pos, def, part.position, partById(part.defId))) return false;
    return true;
}
bool VABSystem::hasSupport(int defId, GridPoint pos) const {
    // The workshop floor can support any part at its geometric base, even an engine
    // whose only physical attachment node is on top.
    if (pos.y + partById(defId).height == 0) return true;
    for (const auto& own : partById(defId).nodes) {
        GridPoint p{pos.x + own.offset.x, pos.y + own.offset.y};
        if (own.direction == Direction::Down && p.y == 0) return true;
        for (const auto& part : parts_) for (const auto& node : partById(part.defId).nodes) {
            GridPoint other{part.position.x + node.offset.x, part.position.y + node.offset.y};
            if (same(p, other) && opposite(own.direction, node.direction) && !nodeOccupied(other, node.direction)) return true;
        }
    }
    return false;
}
Placement VABSystem::preview(int defId, double cursorX, double cursorY, bool symmetry) const {
    const auto& def = partById(defId);
    GridPoint desired{static_cast<int>(std::round(cursorX - def.width / 2.0)), static_cast<int>(std::round(cursorY - def.height / 2.0))};
    Placement result{desired, {}, false, false};
    double best = 1.5 * 1.5; // magnetic radius, in grid units
    double bestInvalid = best;
    auto consider = [&](GridPoint target, const AttachNode& own, Direction targetDirection) {
        if (!opposite(own.direction, targetDirection)) return;
        GridPoint pos{target.x - own.offset.x, target.y - own.offset.y};
        double dx = pos.x - (cursorX - def.width / 2.0);
        double dy = pos.y - (cursorY - def.height / 2.0);
        double distance = dx*dx + dy*dy;
        if (distance > 1.5 * 1.5) return;
        GridPoint mirrored = mirror(pos, def);
        bool valid = canPlace(defId, pos) && (!symmetry || same(pos, mirrored) ||
            (canPlace(defId, mirrored) && !overlap(pos, def, mirrored, def) && hasSupport(defId, mirrored)));
        if (!valid) {
            if (!result.valid && distance <= bestInvalid) { bestInvalid = distance; result = {pos, target, true, false}; }
            return;
        }
        if (distance > best) return;
        best = distance;
        result = {pos, target, true, true};
    };
    // Floor anchors are structural supports rather than attachment nodes on the part.
    AttachNode floorBase{{def.width / 2, def.height}, Direction::Down};
    consider({desired.x + floorBase.offset.x, 0}, floorBase, Direction::Up);
    for (const auto& own : def.nodes) {
        for (const auto& part : parts_) for (const auto& node : partById(part.defId).nodes) {
            GridPoint p{part.position.x + node.offset.x, part.position.y + node.offset.y};
            if (!nodeOccupied(p, node.direction)) consider(p, own, node.direction);
        }
    }
    return result;
}
bool VABSystem::place(int defId, const Placement& p, bool symmetry) {
    if (!p.valid || !p.snapped || !canPlace(defId, p.position) || !hasSupport(defId, p.position)) return false;
    const auto& def = partById(defId);
    GridPoint mirrored = mirror(p.position, def);
    if (symmetry && !same(p.position, mirrored) && (!canPlace(defId, mirrored) || overlap(p.position, def, mirrored, def) || !hasSupport(defId, mirrored))) return false;
    parts_.push_back({defId, p.position, nextUid_++});
    if (symmetry && !same(p.position, mirrored)) parts_.push_back({defId, mirrored, nextUid_++});
    return true;
}
std::optional<PartInstance> VABSystem::removeAt(double x, double y) {
    for (auto it = parts_.rbegin(); it != parts_.rend(); ++it) {
        const auto& d = partById(it->defId);
        if (x >= it->position.x && x < it->position.x + d.width && y >= it->position.y && y < it->position.y + d.height) {
            PartInstance result = *it;
            parts_.erase(std::next(it).base());
            return result;
        }
    }
    return std::nullopt;
}
bool VABSystem::removeById(unsigned long long uid) {
    for (auto it = parts_.begin(); it != parts_.end(); ++it) if (it->uid == uid) { parts_.erase(it); return true; }
    return false;
}
bool VABSystem::restore(const PartInstance& part) {
    if (!canPlace(part.defId, part.position)) return false;
    parts_.push_back(part);
    return true;
}
Telemetry VABSystem::telemetry() const {
    Telemetry t;
    for (const auto& part : parts_) { const auto& d = partById(part.defId); t.mass += d.dryMass + d.fuelCapacity; t.thrust += d.thrust; ++t.count; }
    if (t.mass > 0) t.twr = t.thrust / (t.mass * 9.80665);
    return t;
}

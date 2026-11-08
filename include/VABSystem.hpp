#pragma once
#include "PartCatalog.hpp"
#include <optional>
#include <vector>

struct Telemetry { double mass = 0, thrust = 0, twr = 0; int count = 0; };
struct Placement {
    GridPoint position{};
    GridPoint snapPoint{};
    bool snapped = false;
    bool valid = false;
};
class VABSystem {
public:
    const std::vector<PartInstance>& parts() const { return parts_; }
    // Cursor is in continuous grid coordinates, relative to the floor/axis origin.
    Placement preview(int defId, double cursorX, double cursorY, bool symmetry) const;
    bool place(int defId, const Placement& placement, bool symmetry);
    std::optional<PartInstance> removeAt(double x, double y);
    bool restore(const PartInstance& part);
    bool removeById(unsigned long long uid);
    Telemetry telemetry() const;
    void clear() { parts_.clear(); }
private:
    std::vector<PartInstance> parts_;
    unsigned long long nextUid_ = 1;
    bool nodeOccupied(GridPoint point, Direction direction) const;
    bool canPlace(int defId, GridPoint pos) const;
    bool hasSupport(int defId, GridPoint pos) const;
};

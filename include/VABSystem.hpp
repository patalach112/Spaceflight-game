#pragma once
#include "PartGeometry.hpp"
#include <optional>
#include <vector>

struct Telemetry { double mass = 0, thrust = 0, twr = 0; int count = 0; };
struct CenterOfMass { double x = 0, y = 0, mass = 0; };
struct Placement {
    GridPoint position{};
    GridPoint snapPoint{};
    bool snapped = false;
    bool valid = false;
    int rotation = 0;
    bool flipped = false;
};
class AEGIS_API VABSystem {
public:
    const std::vector<PartInstance>& parts() const { return parts_; }
    // Cursor is in continuous grid coordinates, relative to the reference-axis origin.
    Placement preview(int defId, double cursorX, double cursorY, bool symmetry, int rotation = 0, bool flipped = false, double progress = 0) const;
    bool place(int defId, const Placement& placement, bool symmetry, bool deployed = false, double progress = -1);
    std::optional<PartInstance> removeAt(double x, double y);
    bool restore(const PartInstance& part);
    bool removeById(unsigned long long uid);
    Telemetry telemetry() const;
    CenterOfMass centerOfMass() const;
    bool rotateById(unsigned long long uid, int direction);
    const PartInstance* findById(unsigned long long uid) const;
    bool toggleDeploymentById(unsigned long long uid);
    void updateAnimations(double seconds);
    void clear() { parts_.clear(); }
private:
    std::vector<PartInstance> parts_;
    unsigned long long nextUid_ = 1;
    bool nodeOccupied(GridPoint point, Direction direction) const;
    bool canPlace(int defId, GridPoint pos, int rotation, bool flipped, double progress, unsigned long long ignore = 0) const;
};

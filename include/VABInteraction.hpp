#pragma once
#include "VABSystem.hpp"
#include <optional>
// Input-independent drag state. Palette pickup and placed-part pickup both end on release.
class AEGIS_API VABInteraction {
public:
    explicit VABInteraction(VABSystem& system) : system_(system) {}
    void choosePart(int id);
    bool active() const { return dragId_ >= 0; }
    int partId() const { return dragId_; }
    int rotation() const { return rotation_; }
    bool flipped() const { return flipped_; }
    bool deployed() const;
    bool canToggleDeployment() const;
    bool toggleDeployment();
    bool moving() const { return moved_.has_value(); }
    unsigned long long selected() const { return selected_; }
    Placement preview(double x, double y, bool symmetry) const;
    double ghostX(double x) const;
    double ghostY(double y) const;
    Telemetry telemetry() const;
    CenterOfMass centerOfMass() const;
    bool finishDrag(double x, double y, bool inWorkspace, bool symmetry, bool dragged);
    void beginMove(double x, double y);
    bool rotate(int direction);
    void cancel();
    void clear();
    void eraseSelectedOrAt(double x, double y);
private:
    VABSystem& system_;
    int dragId_ = -1, rotation_ = 0;
    bool flipped_ = false, dragDeployed_ = false;
    std::optional<PartInstance> moved_;
    unsigned long long selected_ = 0;
    double offsetX_ = 0, offsetY_ = 0;
};

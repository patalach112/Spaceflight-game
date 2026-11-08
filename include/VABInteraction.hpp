#pragma once
#include "VABSystem.hpp"
#include <optional>

// Input-independent interaction state: the window layer supplies grid coordinates
// and click/drag events. A palette click arms a ghost; only a later workspace click commits it.
class AEGIS_API VABInteraction {
public:
    explicit VABInteraction(VABSystem& system) : system_(system) {}
    void choosePart(int id);
    bool active() const { return dragId_ >= 0; }
    int partId() const { return dragId_; }
    bool moving() const { return moved_.has_value(); }
    unsigned long long selected() const { return selected_; }
    Placement preview(double x, double y, bool symmetry) const;
    Telemetry telemetry() const;
    bool placeClick(double x, double y, bool symmetry);
    void beginMove(double x, double y);
    void endMove(double x, double y, bool dragged);
    void cancel();
    void eraseSelectedOrAt(double x, double y);
private:
    VABSystem& system_;
    int dragId_ = -1;
    std::optional<PartInstance> moved_;
    unsigned long long selected_ = 0;
    double offsetX_ = 0, offsetY_ = 0;
};

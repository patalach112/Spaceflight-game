#include "PartCatalog.hpp"
#include <stdexcept>

const std::vector<PartDef>& partCatalog() {
    // Thrust is stored in kN. Published SFS engine thrust figures are in
    // tonnes-force; 1 tf is converted using standard gravity (9.80665 kN).
    static const std::vector<PartDef> catalog = {
        {0, "CAPSULE", Category::Pods, 4, 4, 4.0, 0, 0, {{{2,0},Direction::Up},{{2,4},Direction::Down}}, PartShape::Capsule},
        {4, "PROBE", Category::Other, 4, 2, 3.0, 0, 0, {{{2,0},Direction::Up},{{2,2},Direction::Down}}, PartShape::Probe},
        {5, "4X2 FUEL TANK", Category::FuelTanks, 4, 2, .5, 4.5, 0, {{{2,0},Direction::Up},{{2,2},Direction::Down},{{0,1},Direction::Left},{{4,1},Direction::Right}}, PartShape::Tank},
        {6, "4X4 FUEL TANK", Category::FuelTanks, 4, 4, 1.0, 9.0, 0, {{{2,0},Direction::Up},{{2,4},Direction::Down},{{0,2},Direction::Left},{{4,2},Direction::Right}}, PartShape::Tank},
        {1, "4X6 FUEL TANK", Category::FuelTanks, 4, 6, 1.5, 13.5, 0, {{{2,0},Direction::Up},{{2,6},Direction::Down},{{0,3},Direction::Left},{{4,3},Direction::Right}}, PartShape::Tank},
        {7, "4X8 FUEL TANK", Category::FuelTanks, 4, 8, 2.0, 18.0, 0, {{{2,0},Direction::Up},{{2,8},Direction::Down},{{0,4},Direction::Left},{{4,4},Direction::Right}}, PartShape::Tank},
        // Taller engine footprints keep the full-width mounting flange while
        // giving the chamber and bell their intended vertical proportions.
        {8, "VALIANT ENGINE", Category::Engines, 4, 6, 2.0, 0, 40*9.80665, {{{2,0},Direction::Up}}, PartShape::Engine},
        {9, "TITAN ENGINE", Category::Engines, 6, 9, 12.0, 0, 400*9.80665, {{{3,0},Direction::Up}}, PartShape::Engine},
        {2, "HAWK ENGINE", Category::Engines, 4, 6, 3.5, 0, 120*9.80665, {{{2,0},Direction::Up}}, PartShape::Engine},
        {10, "FRONTIER ENGINE", Category::Engines, 6, 9, 6.0, 0, 100*9.80665, {{{3,0},Direction::Up}}, PartShape::Engine},
        {11, "KOLIBRI ENGINE", Category::Engines, 2, 3, .5, 0, 15*9.80665, {{{1,0},Direction::Up}}, PartShape::Engine},
        {12, "NOSE CONE", Category::Aerodynamics, 4, 4, .25, 0, 0, {{{2,4},Direction::Down}}, PartShape::NoseCone},
        {3, "SEPARATOR", Category::Structural, 4, 1, .4, 0, 0, {{{2,0},Direction::Up},{{2,1},Direction::Down}}, PartShape::Decoupler},
        {13, "SIDE SEPARATOR", Category::Structural, 2, 2, .2, 0, 0, {{{0,1},Direction::Left},{{2,1},Direction::Right}}, PartShape::SideSeparator},
        {14, "LANDING LEG", Category::Other, 2, 4, .5, 0, 0, {{{0,2},Direction::Left}}, PartShape::LandingLeg},
        {15, "RCS THRUSTERS", Category::Other, 2, 2, .05, 0, 1.5*9.80665, {{{0,1},Direction::Left}}, PartShape::Rcs},
        {16, "PANEL", Category::Structural, 4, 1, .2, 0, 0, {{{2,0},Direction::Up},{{2,1},Direction::Down}}, PartShape::Panel},
        {17, "TRUSS", Category::Structural, 4, 2, .3, 0, 0, {{{2,0},Direction::Up},{{2,2},Direction::Down},{{0,1},Direction::Left},{{4,1},Direction::Right}}, PartShape::Truss},
        {18, "SIDE NOSE CONE", Category::Aerodynamics, 2, 4, .25, 0, 0, {{{0,2},Direction::Left}}, PartShape::SideNose},
        {19, "PARACHUTE", Category::Aerodynamics, 2, 1, .4, 0, 0, {{{1,1},Direction::Down}}, PartShape::Parachute},
        {20, "HEAT SHIELD", Category::Aerodynamics, 4, 1, .25, 0, 0, {{{2,0},Direction::Up},{{2,1},Direction::Down}}, PartShape::HeatShield},
        {21, "DOCKING PORT", Category::Other, 2, 1, .19, 0, 0, {{{1,0},Direction::Up},{{1,1},Direction::Down}}, PartShape::DockingPort},
        {22, "6X2 FUEL TANK", Category::FuelTanks, 6, 2, 0.75, 6.75, 0, {{{3,0},Direction::Up},{{3,2},Direction::Down},{{0,1},Direction::Left},{{6,1},Direction::Right}}, PartShape::Tank},
        {23, "6X4 FUEL TANK", Category::FuelTanks, 6, 4, 1.5, 13.5, 0, {{{3,0},Direction::Up},{{3,4},Direction::Down},{{0,2},Direction::Left},{{6,2},Direction::Right}}, PartShape::Tank},
        {24, "6X6 FUEL TANK", Category::FuelTanks, 6, 6, 2.25, 20.25, 0, {{{3,0},Direction::Up},{{3,6},Direction::Down},{{0,3},Direction::Left},{{6,3},Direction::Right}}, PartShape::Tank},
        {25, "6X8 FUEL TANK", Category::FuelTanks, 6, 8, 3, 27, 0, {{{3,0},Direction::Up},{{3,8},Direction::Down},{{0,4},Direction::Left},{{6,4},Direction::Right}}, PartShape::Tank},
        {26, "6X16 FUEL TANK", Category::FuelTanks, 6, 16, 6, 54, 0, {{{3,0},Direction::Up},{{3,16},Direction::Down},{{0,8},Direction::Left},{{6,8},Direction::Right}}, PartShape::Tank},
        {27, "8X2 FUEL TANK", Category::FuelTanks, 8, 2, 1, 9, 0, {{{4,0},Direction::Up},{{4,2},Direction::Down},{{0,1},Direction::Left},{{8,1},Direction::Right}}, PartShape::Tank},
        {28, "8X4 FUEL TANK", Category::FuelTanks, 8, 4, 2, 18, 0, {{{4,0},Direction::Up},{{4,4},Direction::Down},{{0,2},Direction::Left},{{8,2},Direction::Right}}, PartShape::Tank},
        {29, "8X6 FUEL TANK", Category::FuelTanks, 8, 6, 3, 27, 0, {{{4,0},Direction::Up},{{4,6},Direction::Down},{{0,3},Direction::Left},{{8,3},Direction::Right}}, PartShape::Tank},
        {30, "8X8 FUEL TANK", Category::FuelTanks, 8, 8, 4, 36, 0, {{{4,0},Direction::Up},{{4,8},Direction::Down},{{0,4},Direction::Left},{{8,4},Direction::Right}}, PartShape::Tank},
        {31, "8X16 FUEL TANK", Category::FuelTanks, 8, 16, 8, 72, 0, {{{4,0},Direction::Up},{{4,16},Direction::Down},{{0,8},Direction::Left},{{8,8},Direction::Right}}, PartShape::Tank},
        {32, "10X2 FUEL TANK", Category::FuelTanks, 10, 2, 1.25, 11.25, 0, {{{5,0},Direction::Up},{{5,2},Direction::Down},{{0,1},Direction::Left},{{10,1},Direction::Right}}, PartShape::Tank},
        {33, "10X4 FUEL TANK", Category::FuelTanks, 10, 4, 2.5, 22.5, 0, {{{5,0},Direction::Up},{{5,4},Direction::Down},{{0,2},Direction::Left},{{10,2},Direction::Right}}, PartShape::Tank},
        {34, "10X6 FUEL TANK", Category::FuelTanks, 10, 6, 3.75, 33.75, 0, {{{5,0},Direction::Up},{{5,6},Direction::Down},{{0,3},Direction::Left},{{10,3},Direction::Right}}, PartShape::Tank},
        {35, "10X8 FUEL TANK", Category::FuelTanks, 10, 8, 5, 45, 0, {{{5,0},Direction::Up},{{5,8},Direction::Down},{{0,4},Direction::Left},{{10,4},Direction::Right}}, PartShape::Tank},
        {36, "10X16 FUEL TANK", Category::FuelTanks, 10, 16, 10, 90, 0, {{{5,0},Direction::Up},{{5,16},Direction::Down},{{0,8},Direction::Left},{{10,8},Direction::Right}}, PartShape::Tank},
        {37, "12X2 FUEL TANK", Category::FuelTanks, 12, 2, 1.5, 13.5, 0, {{{6,0},Direction::Up},{{6,2},Direction::Down},{{0,1},Direction::Left},{{12,1},Direction::Right}}, PartShape::Tank},
        {38, "12X4 FUEL TANK", Category::FuelTanks, 12, 4, 3, 27, 0, {{{6,0},Direction::Up},{{6,4},Direction::Down},{{0,2},Direction::Left},{{12,2},Direction::Right}}, PartShape::Tank},
        {39, "12X6 FUEL TANK", Category::FuelTanks, 12, 6, 4.5, 40.5, 0, {{{6,0},Direction::Up},{{6,6},Direction::Down},{{0,3},Direction::Left},{{12,3},Direction::Right}}, PartShape::Tank},
        {40, "12X8 FUEL TANK", Category::FuelTanks, 12, 8, 6, 54, 0, {{{6,0},Direction::Up},{{6,8},Direction::Down},{{0,4},Direction::Left},{{12,4},Direction::Right}}, PartShape::Tank},
        {41, "12X16 FUEL TANK", Category::FuelTanks, 12, 16, 12, 108, 0, {{{6,0},Direction::Up},{{6,16},Direction::Down},{{0,8},Direction::Left},{{12,8},Direction::Right}}, PartShape::Tank},
        {42, "6W NOSE CONE", Category::Aerodynamics, 6, 6, 0.375, 0, 0, {{{3,6},Direction::Down}}, PartShape::NoseCone},
        {43, "8W NOSE CONE", Category::Aerodynamics, 8, 8, 0.5, 0, 0, {{{4,8},Direction::Down}}, PartShape::NoseCone},
        {44, "10W NOSE CONE", Category::Aerodynamics, 10, 10, 0.625, 0, 0, {{{5,10},Direction::Down}}, PartShape::NoseCone},
        {45, "12W NOSE CONE", Category::Aerodynamics, 12, 12, 0.75, 0, 0, {{{6,12},Direction::Down}}, PartShape::NoseCone},
        {46, "6 TO 4 ADAPTER", Category::Structural, 6, 2, 0.6, 0, 0, {{{3,0},Direction::Up},{{3,2},Direction::Down}}, PartShape::Adapter},
        {47, "8 TO 6 ADAPTER", Category::Structural, 8, 2, 0.8, 0, 0, {{{4,0},Direction::Up},{{4,2},Direction::Down}}, PartShape::Adapter},
        {48, "10 TO 8 ADAPTER", Category::Structural, 10, 2, 1, 0, 0, {{{5,0},Direction::Up},{{5,2},Direction::Down}}, PartShape::Adapter},
        {49, "12 TO 10 ADAPTER", Category::Structural, 12, 2, 1.2, 0, 0, {{{6,0},Direction::Up},{{6,2},Direction::Down}}, PartShape::Adapter},
        {50, "6W SEPARATOR", Category::Structural, 6, 1, 0.6, 0, 0, {{{3,0},Direction::Up},{{3,1},Direction::Down}}, PartShape::Decoupler},
        {51, "8W SEPARATOR", Category::Structural, 8, 1, 0.8, 0, 0, {{{4,0},Direction::Up},{{4,1},Direction::Down}}, PartShape::Decoupler},
        {52, "10W SEPARATOR", Category::Structural, 10, 1, 1, 0, 0, {{{5,0},Direction::Up},{{5,1},Direction::Down}}, PartShape::Decoupler},
        {53, "12W SEPARATOR", Category::Structural, 12, 1, 1.2, 0, 0, {{{6,0},Direction::Up},{{6,1},Direction::Down}}, PartShape::Decoupler},
        {54, "6W HEAT SHIELD", Category::Aerodynamics, 6, 1, 0.375, 0, 0, {{{3,0},Direction::Up},{{3,1},Direction::Down}}, PartShape::HeatShield},
        {55, "8W HEAT SHIELD", Category::Aerodynamics, 8, 1, 0.5, 0, 0, {{{4,0},Direction::Up},{{4,1},Direction::Down}}, PartShape::HeatShield},
        {56, "10W HEAT SHIELD", Category::Aerodynamics, 10, 1, 0.625, 0, 0, {{{5,0},Direction::Up},{{5,1},Direction::Down}}, PartShape::HeatShield},
        {57, "12W HEAT SHIELD", Category::Aerodynamics, 12, 1, 0.75, 0, 0, {{{6,0},Direction::Up},{{6,1},Direction::Down}}, PartShape::HeatShield},
        {58, "4W DOCKING PORT", Category::Other, 4, 1, 0.38, 0, 0, {{{2,0},Direction::Up},{{2,1},Direction::Down}}, PartShape::DockingPort},
        {59, "6W DOCKING PORT", Category::Other, 6, 1, 0.57, 0, 0, {{{3,0},Direction::Up},{{3,1},Direction::Down}}, PartShape::DockingPort},
        {60, "8W TRUSS", Category::Structural, 8, 2, 0.6, 0, 0, {{{4,0},Direction::Up},{{4,2},Direction::Down},{{0,1},Direction::Left},{{8,1},Direction::Right}}, PartShape::Truss},
        {61, "12W TRUSS", Category::Structural, 12, 2, 0.9, 0, 0, {{{6,0},Direction::Up},{{6,2},Direction::Down},{{0,1},Direction::Left},{{12,1},Direction::Right}}, PartShape::Truss},
        {62, "SIDE FAIRING 4", Category::Fairings, 2, 4, 0.2, 0, 0, {{{0,2},Direction::Left},{{1,4},Direction::Down}}, PartShape::Fairing},
        {63, "SIDE FAIRING 8", Category::Fairings, 2, 8, 0.4, 0, 0, {{{0,4},Direction::Left},{{1,8},Direction::Down}}, PartShape::Fairing},
        {64, "SIDE FAIRING 12", Category::Fairings, 2, 12, 0.6, 0, 0, {{{0,6},Direction::Left},{{1,12},Direction::Down}}, PartShape::Fairing},
        {65, "SIDE FAIRING 16", Category::Fairings, 2, 16, 0.8, 0, 0, {{{0,8},Direction::Left},{{1,16},Direction::Down}}, PartShape::Fairing},
        {66, "SMALL ROVER WHEEL", Category::Other, 2, 2, 0.3, 0, 0, {{{0,1},Direction::Left}}, PartShape::Wheel},
        {67, "LARGE ROVER WHEEL", Category::Other, 3, 3, 0.5, 0, 0, {{{0,1},Direction::Left}}, PartShape::Wheel},
        {68, "HEAVY LANDING LEG", Category::Other, 4, 6, 0.75, 0, 0, {{{0,3},Direction::Left}}, PartShape::HeavyLeg},
        {69, "ION ENGINE", Category::Engines, 2, 3, .5, 0, 1.5*9.80665, {{{1,0},Direction::Up}}, PartShape::IonEngine},
        {70, "6W ENGINE BASE", Category::FuelTanks, 6, 5, 1.875, 16.875, 0, {{{3,0},Direction::Up},{{3,5},Direction::Down},{{0,2},Direction::Left},{{6,2},Direction::Right}}, PartShape::EngineBase},
        {71, "8W ENGINE BASE", Category::FuelTanks, 8, 5, 2.5, 22.5, 0, {{{4,0},Direction::Up},{{4,5},Direction::Down},{{0,2},Direction::Left},{{8,2},Direction::Right}}, PartShape::EngineBase},
        {72, "10W ENGINE BASE", Category::FuelTanks, 10, 5, 3.125, 28.125, 0, {{{5,0},Direction::Up},{{5,5},Direction::Down},{{0,2},Direction::Left},{{10,2},Direction::Right}}, PartShape::EngineBase},
        {73, "12W ENGINE BASE", Category::FuelTanks, 12, 5, 3.75, 33.75, 0, {{{6,0},Direction::Up},{{6,5},Direction::Down},{{0,2},Direction::Left},{{12,2},Direction::Right}}, PartShape::EngineBase},
        {74, "SMALL SOLAR PANEL", Category::Other, 2, 4, 0.2, 0, 0, {{{0,2},Direction::Left}}, PartShape::SolarPanel},
        {75, "LARGE SOLAR PANEL", Category::Other, 4, 6, 0.4, 0, 0, {{{0,3},Direction::Left}}, PartShape::SolarPanel},
        {76, "SMALL FIN", Category::Aerodynamics, 2, 4, 0.1, 0, 0, {{{0,2},Direction::Left}}, PartShape::Fin},
        {77, "LARGE FIN", Category::Aerodynamics, 4, 6, 0.3, 0, 0, {{{0,3},Direction::Left}}, PartShape::Fin},
        {78, "6W HOLLOW SHIELD", Category::Aerodynamics, 6, 1, 0.3, 0, 0, {{{3,0},Direction::Up},{{3,1},Direction::Down}}, PartShape::HollowShield},
        {79, "8W HOLLOW SHIELD", Category::Aerodynamics, 8, 1, 0.4, 0, 0, {{{4,0},Direction::Up},{{4,1},Direction::Down}}, PartShape::HollowShield},
        {80, "10W HOLLOW SHIELD", Category::Aerodynamics, 10, 1, 0.5, 0, 0, {{{5,0},Direction::Up},{{5,1},Direction::Down}}, PartShape::HollowShield},
        {81, "12W HOLLOW SHIELD", Category::Aerodynamics, 12, 1, 0.6, 0, 0, {{{6,0},Direction::Up},{{6,1},Direction::Down}}, PartShape::HollowShield},
        {82, "SIDE PARACHUTE", Category::Aerodynamics, 2, 2, .4, 0, 0, {{{0,1},Direction::Left}}, PartShape::SideChute},
    };
    return catalog;
}
bool partInCategory(const PartDef& def, Category category) {
    return def.category==category || (category==Category::Engines && def.shape==PartShape::Rcs) ||
           (category==Category::Other && def.shape==PartShape::IonEngine) ||
           (category==Category::Pods && def.shape==PartShape::Probe);
}
const PartDef& partById(int id) {
    for (const auto& def : partCatalog()) if (def.id == id) return def;
    throw std::out_of_range("Unknown part ID");
}

int normalizedRotation(int rotation) { return (rotation % 4 + 4) % 4; }
int partWidth(const PartDef& def, int rotation) { return normalizedRotation(rotation) % 2 ? def.height : def.width; }
int partHeight(const PartDef& def, int rotation) { return normalizedRotation(rotation) % 2 ? def.width : def.height; }
AttachNode rotatedNode(const PartDef& def, AttachNode node, int rotation, bool flipped) {
    if (flipped) {
        node.offset.x = def.width - node.offset.x;
        if (node.direction == Direction::Left) node.direction = Direction::Right;
        else if (node.direction == Direction::Right) node.direction = Direction::Left;
    }
    int w = def.width, h = def.height;
    for (int i = 0; i < normalizedRotation(rotation); ++i) {
        node.offset = {h - node.offset.y, node.offset.x};
        switch (node.direction) {
            case Direction::Up: node.direction = Direction::Right; break;
            case Direction::Right: node.direction = Direction::Down; break;
            case Direction::Down: node.direction = Direction::Left; break;
            case Direction::Left: node.direction = Direction::Up; break;
        }
        int temp = w; w = h; h = temp;
    }
    return node;
}

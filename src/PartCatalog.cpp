#include "PartCatalog.hpp"
#include <stdexcept>

const std::vector<PartDef>& partCatalog() {
    // Thrust is stored in kN. Published SFS engine thrust figures are in
    // tonnes-force; 1 tf is converted using standard gravity (9.80665 kN).
    static const std::vector<PartDef> catalog = {
        {0, "COMMAND POD", Category::Pods, 4, 4, 4.0, 0, 0, {{{2,4},Direction::Down}}, PartShape::Capsule},
        {4, "PROBE CORE", Category::Pods, 4, 2, 3.0, 0, 0, {{{2,0},Direction::Up},{{2,2},Direction::Down}}, PartShape::Probe},
        {5, "TINY TANK", Category::FuelTanks, 4, 2, .5, 4.5, 0, {{{2,0},Direction::Up},{{2,2},Direction::Down},{{0,1},Direction::Left},{{4,1},Direction::Right}}, PartShape::Tank},
        {6, "SMALL TANK", Category::FuelTanks, 4, 4, 1.0, 9.0, 0, {{{2,0},Direction::Up},{{2,4},Direction::Down},{{0,2},Direction::Left},{{4,2},Direction::Right}}, PartShape::Tank},
        {1, "MEDIUM TANK", Category::FuelTanks, 4, 6, 1.5, 13.5, 0, {{{2,0},Direction::Up},{{2,6},Direction::Down},{{0,3},Direction::Left},{{4,3},Direction::Right}}, PartShape::Tank},
        {7, "LARGE TANK", Category::FuelTanks, 4, 8, 2.0, 18.0, 0, {{{2,0},Direction::Up},{{2,8},Direction::Down},{{0,4},Direction::Left},{{4,4},Direction::Right}}, PartShape::Tank},
        {8, "VALIANT", Category::Engines, 4, 3, 2.0, 0, 40*9.80665, {{{2,0},Direction::Up}}, PartShape::Engine},
        {9, "TITAN", Category::Engines, 6, 4, 12.0, 0, 400*9.80665, {{{3,0},Direction::Up}}, PartShape::Engine},
        {2, "HAWK", Category::Engines, 4, 3, 3.5, 0, 120*9.80665, {{{2,0},Direction::Up}}, PartShape::Engine},
        {10, "FRONTIER", Category::Engines, 6, 4, 6.0, 0, 100*9.80665, {{{3,0},Direction::Up}}, PartShape::Engine},
        {11, "KOLIBRI", Category::Engines, 2, 3, .5, 0, 15*9.80665, {{{1,0},Direction::Up}}, PartShape::Engine},
        {12, "NOSE CONE", Category::Structural, 4, 3, .1, 0, 0, {{{2,3},Direction::Down}}, PartShape::NoseCone},
        {3, "DECOUPLER", Category::Structural, 4, 1, .3, 0, 0, {{{2,0},Direction::Up},{{2,1},Direction::Down}}, PartShape::Decoupler},
        {13, "SIDE SEPARATOR", Category::Structural, 2, 2, .1, 0, 0, {{{0,1},Direction::Left},{{2,1},Direction::Right}}, PartShape::SideSeparator},
        {14, "LANDING LEG", Category::Structural, 2, 4, .25, 0, 0, {{{0,2},Direction::Left}}, PartShape::LandingLeg},
        {15, "RCS THRUSTER", Category::Structural, 2, 2, .05, 0, 1.5*9.80665, {{{0,1},Direction::Left}}, PartShape::Rcs},
        {16, "PANEL", Category::Structural, 4, 1, .15, 0, 0, {{{2,0},Direction::Up},{{2,1},Direction::Down}}, PartShape::Panel},
        {17, "TRUSS", Category::Structural, 4, 2, .3, 0, 0, {{{2,0},Direction::Up},{{2,2},Direction::Down},{{0,1},Direction::Left},{{4,1},Direction::Right}}, PartShape::Truss},
        {18, "SIDE NOSE", Category::Structural, 2, 4, .1, 0, 0, {{{0,2},Direction::Left}}, PartShape::SideNose},
        {19, "PARACHUTE", Category::Structural, 2, 1, .1, 0, 0, {{{1,1},Direction::Down}}, PartShape::Parachute},
        {20, "HEAT SHIELD", Category::Structural, 4, 1, .3, 0, 0, {{{2,0},Direction::Up},{{2,1},Direction::Down}}, PartShape::HeatShield},
        {21, "DOCKING PORT", Category::Structural, 2, 1, .2, 0, 0, {{{1,0},Direction::Up},{{1,1},Direction::Down}}, PartShape::DockingPort}
    };
    return catalog;
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

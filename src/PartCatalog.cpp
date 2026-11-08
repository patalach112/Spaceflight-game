#include "PartCatalog.hpp"
#include <stdexcept>

const std::vector<PartDef>& partCatalog() {
    static const std::vector<PartDef> catalog = {
        {0, "COMMAND POD", Category::Pods, 4, 4, 0.80, 0.0, 0.0, {{{2,4}, Direction::Down}}},
        {1, "FUEL TANK", Category::FuelTanks, 4, 6, 0.50, 4.0, 0.0, {{{2,0}, Direction::Up}, {{2,6}, Direction::Down}}},
        {2, "LIQUID ENGINE", Category::Engines, 4, 3, 0.65, 0.0, 120.0, {{{2,0}, Direction::Up}}},
        {3, "DECOUPLER", Category::Structural, 4, 1, 0.12, 0.0, 0.0, {{{2,0}, Direction::Up}, {{2,1}, Direction::Down}}}
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
AttachNode rotatedNode(const PartDef& def, AttachNode node, int rotation) {
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

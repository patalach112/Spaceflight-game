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

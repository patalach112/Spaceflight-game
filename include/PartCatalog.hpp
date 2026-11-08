#pragma once
#include "Export.hpp"
#include <string_view>
#include <vector>

enum class Category { Pods, FuelTanks, Engines, Structural };
enum class Direction { Up, Down, Left, Right };
struct GridPoint { int x = 0; int y = 0; };
struct AttachNode { GridPoint offset; Direction direction; };
struct PartDef {
    int id;
    std::string_view name;
    Category category;
    int width, height; // grid units; one unit is 0.5 m
    double dryMass, fuelCapacity, thrust; // tonnes, tonnes, kN
    std::vector<AttachNode> nodes;
};
struct PartInstance { int defId; GridPoint position; unsigned long long uid; };
AEGIS_API const std::vector<PartDef>& partCatalog();
AEGIS_API const PartDef& partById(int id);

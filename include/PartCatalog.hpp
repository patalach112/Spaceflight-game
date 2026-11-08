#pragma once
#include "Export.hpp"
#include <string_view>
#include <vector>

enum class Category { Pods, FuelTanks, Engines, Structural, Aerodynamics, Fairings, Other };
enum class Direction { Up, Down, Left, Right };
enum class PartShape { Capsule, Probe, Tank, Engine, NoseCone, Decoupler, SideSeparator, LandingLeg, Rcs, Panel, Truss, SideNose, Parachute, HeatShield, DockingPort, Adapter, Wheel, HeavyLeg, IonEngine, Fairing, EngineBase, SolarPanel, Fin, HollowShield, SideChute };
struct GridPoint { int x = 0; int y = 0; };
struct AttachNode { GridPoint offset; Direction direction; };
struct PartDef {
    int id;
    std::string_view name;
    Category category;
    int width, height; // grid units; one unit is 0.5 m
    double dryMass, fuelCapacity, thrust; // tonnes, tonnes, kN
    std::vector<AttachNode> nodes;
    PartShape shape;
};
struct PartInstance { int defId; GridPoint position; unsigned long long uid; int rotation = 0; bool flipped = false; };
// Rotation is clockwise in 90 degree steps, around the part's bounding box.
AEGIS_API int normalizedRotation(int rotation);
AEGIS_API int partWidth(const PartDef& def, int rotation);
AEGIS_API int partHeight(const PartDef& def, int rotation);
AEGIS_API AttachNode rotatedNode(const PartDef& def, AttachNode node, int rotation, bool flipped = false);
// A few parts intentionally appear in more than one catalog tab.
AEGIS_API bool partInCategory(const PartDef& def, Category category);
AEGIS_API const std::vector<PartDef>& partCatalog();
AEGIS_API const PartDef& partById(int id);

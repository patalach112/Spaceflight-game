#pragma once
#include "PartCatalog.hpp"
#include <array>
#include <vector>

// Local coordinates are normalized to the part's unrotated catalog footprint.
// These convex pieces are the single source for deployable artwork and hitboxes.
struct GeometryPoint { double x, y; };
enum class GeometryMaterial { Mount, Arm, Link, Foot, Solar };
struct GeometryPiece { std::array<GeometryPoint,4> vertices; GeometryMaterial material; };
using Polygon = std::vector<GeometryPoint>;
AEGIS_API std::vector<GeometryPiece> deploymentGeometry(const PartDef& def, double progress);
AEGIS_API std::vector<Polygon> partGeometry(const PartDef& def, GridPoint position, int rotation, bool flipped, double progress);
AEGIS_API bool geometryContains(const std::vector<Polygon>& geometry, double x, double y);
AEGIS_API bool geometryOverlaps(const std::vector<Polygon>& a, const std::vector<Polygon>& b);
// Conservative swept convex hulls of corresponding components; no moving
// vertex can pass through another part between animation frames.
AEGIS_API std::vector<Polygon> sweptGeometry(const std::vector<Polygon>& from, const std::vector<Polygon>& to);

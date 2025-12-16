#include "Cell.h"

#include "Grid.h"
#include "../../Renderer/OpenGLRenderer.h"

Cell::Cell(const float height, const float water_depth, const bool is_obstacle, const bool is_river, const bool is_water_source,
           const float river_capacity, const float source_strength)
               : terrainHeight(height),
                 waterDepth(water_depth),
                 velocity(0.0F, 0.0F),
                 obstacle(is_obstacle),
                 river(is_river),
                 waterSource(is_water_source),
                 rainArea(false),
                 riverCapacity(river_capacity),
                 sourceStrength(source_strength),
                 rainIntensity(0.0F)
{
}

void Cell::setTerrainHeight(float height) {
    terrainHeight = std::max(-1.0F * MAX_WATER_DEPTH, height);
}


auto Cell::getType() const -> CellType {
    if (obstacle) return OBSTACLE;
    if (rainArea) return RAIN;
    if (waterSource) return WATER_SOURCE;
    if (river && waterDepth > 0.01F) return RIVER;
    if (terrainHeight > 0.01F) return LAND;
    return EMPTY;
}

void Cell::setType(CellType type) {
    switch (type) {
        case OBSTACLE:
            obstacle = true;
            river = false;
            waterSource = false;
            rainArea = false;
            waterDepth = 0.0f;
            rainIntensity = 0.0f;
            terrainHeight = CAMERA_MAX_HEIGHT;
            break;
        case RIVER:
            river = true;
            obstacle = false;
            waterSource = false;
            rainArea = false;

            // set minimum
            if (waterDepth < 0.1F) {
                waterDepth = 0.5F;
            }
            break;
        case LAND:
            obstacle = false;
            rainArea = false;
            if (terrainHeight < 0.1F) {
                terrainHeight = 1.0F;
            }
            break;
        case WATER_SOURCE:
            waterSource = true;
            obstacle = false;
            river = false;
            rainArea = false;

            // Water source maintains its sourceStrength level
            if (waterDepth < sourceStrength) {
                waterDepth = sourceStrength;
            }
            break;
        case RAIN:
            rainArea = true;
            obstacle = false;
            if (rainIntensity == 0.0F) {
                rainIntensity = 0.5F;
            }
            // rain can be over any terrain so we don't modify other flags
            break;
        case EMPTY:
            obstacle = false;
            river = false;
            waterSource = false;
            rainArea = false;
            terrainHeight = 0.0F;
            waterDepth = 0.0F;
            rainIntensity = 0.0F;
            break;
    }
}

float Cell::getFlowCapacity() const {
    if (obstacle) return 0.0F;

    if (river) {
        return std::max(0.0F, riverCapacity - waterDepth);
    }

    return 1000.0F; // arbitrary large capacity for non-river cells, think of it like a ground that just absorbs water
}

// TODO: we need to keep track of the first values loaded to properly restore them
void Cell::reset() {
    terrainHeight = 0.0F;
    waterDepth = 0.0F;
    velocity = QVector2D(0.0F, 0.0F);
    obstacle = false;
    river = false;
    waterSource = false;
    rainArea = false;
    riverCapacity = 3.0F;
    sourceStrength = 1.0F;
    rainIntensity = 0.0F;
}

void Cell::resetWater() {
    waterDepth = 0.0F;
    velocity = QVector2D(0.0F, 0.0F);
}

// stream operator overloads for data serialization
auto operator<<(QDataStream& stream, const Cell& cell) -> QDataStream& {
    stream << cell.terrainHeight;
    stream << cell.waterDepth;
    stream << cell.velocity;
    stream << cell.obstacle;
    stream << cell.river;
    stream << cell.waterSource;
    stream << cell.rainArea;
    stream << cell.riverCapacity;
    stream << cell.sourceStrength;
    stream << cell.rainIntensity;
    return stream;
}

auto operator>>(QDataStream& stream, Cell& cell) -> QDataStream& {
    stream >> cell.terrainHeight;
    stream >> cell.waterDepth;
    stream >> cell.velocity;
    stream >> cell.obstacle;
    stream >> cell.river;
    stream >> cell.waterSource;
    stream >> cell.rainArea;
    stream >> cell.riverCapacity;
    stream >> cell.sourceStrength;
    stream >> cell.rainIntensity;
    return stream;
}


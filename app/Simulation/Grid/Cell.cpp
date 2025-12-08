#include "Cell.h"

#include "Grid.h"
#include "../../Renderer/OpenGLRenderer.h"

Cell::Cell(float height, float water_depth, bool is_obstacle, bool is_river, bool is_water_source,
           float river_capacity)
               : terrainHeight(height),
                 waterDepth( water_depth),
                 velocity(0.0f, 0.0f),
                 obstacle(is_obstacle),
                 river(is_river),
                 waterSource(is_water_source),
                 riverCapacity(river_capacity)
{
}

void Cell::setTerrainHeight(float height) {
    terrainHeight = std::max(-1.0F * MAX_WATER_DEPTH, height);
}


auto Cell::getType() const -> CellType {
    if (obstacle) return OBSTACLE;
    if (waterDepth > 0.01F && river) return RIVER;
    if (waterDepth > 0.01F && waterSource) return WATER_SOURCE;
    if (terrainHeight > 0.01F) return LAND;
    return EMPTY;
}

void Cell::setType(CellType type) {
    switch (type) {
        case OBSTACLE:
            obstacle = true;
            river = false;
            waterSource = false;
            waterDepth = 0.0f;
            terrainHeight = CAMERA_MAX_HEIGHT;
            break;
        case RIVER:
            river = true;
            obstacle = false;
            waterSource = false;

            // set minimum
            if (waterDepth < 0.1F) {
                waterDepth = 0.5F;
            }
            break;
        case LAND:
            obstacle = false;
            if (terrainHeight < 0.1F) {
                terrainHeight = 1.0F;
            }
            break;
        case EMPTY:
            obstacle = false;
            river = false;
            waterSource = false;
            terrainHeight = 0.0F;
            waterDepth = 0.0F;
            break;
    }
}

float Cell::getFlowCapacity() const {
    if (obstacle) return 0.0F;

    if (river) {
        return std::max(0.0F, riverCapacity - waterDepth);
    }

    return 1000.0F;  // Arbitrary high value for non-river cells
}

void Cell::reset() {
    terrainHeight = 0.0F;
    waterDepth = 0.0F;
    velocity = QVector2D(0.0F, 0.0F);
    obstacle = false;
    river = false;
    waterSource = false;
    riverCapacity = 3.0F;
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
    stream << cell.riverCapacity;
    return stream;
}

auto operator>>(QDataStream& stream, Cell& cell) -> QDataStream& {
    stream >> cell.terrainHeight;
    stream >> cell.waterDepth;
    stream >> cell.velocity;
    stream >> cell.obstacle;
    stream >> cell.river;
    stream >> cell.waterSource;
    stream >> cell.riverCapacity;
    return stream;
}


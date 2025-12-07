#include "Cell.h"

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


CellType Cell::getType() const {
    if (obstacle) return OBSTACLE;
    if (waterDepth > 0.01f) return WATER;  // Small threshold to avoid floating point issues
    if (terrainHeight > 0.01f) return LAND;
    return EMPTY;
}

void Cell::setType(CellType type) {
    switch (type) {
        case OBSTACLE:
            obstacle = true;
            river = false;
            waterSource = false;
            break;
        case WATER:
            obstacle = false;
            // Keep water depth as is or set a minimum
            if (waterDepth < 0.1f) waterDepth = 0.5f;
            break;
        case LAND:
            obstacle = false;
            if (terrainHeight < 0.1f) terrainHeight = 1.0f;
            break;
        case EMPTY:
            obstacle = false;
            river = false;
            waterSource = false;
            terrainHeight = 0.0f;
            waterDepth = 0.0f;
            break;
    }
}

float Cell::getFlowCapacity() const {
    if (obstacle) return 0.0f;

    if (river) {
        return std::max(0.0f, riverCapacity - waterDepth);
    }

    return 1000.0f;
}

void Cell::reset() {
    terrainHeight = 0.0f;
    waterDepth = 0.0f;
    velocity = QVector2D(0.0f, 0.0f);
    obstacle = false;
    river = false;
    waterSource = false;
    riverCapacity = 3.0f;
}

void Cell::resetWater() {
    waterDepth = 0.0f;
    velocity = QVector2D(0.0f, 0.0f);
}

// stream operator overloads for data serialization

QDataStream& operator<<(QDataStream& stream, const Cell& cell) {
    stream << cell.terrainHeight;
    stream << cell.waterDepth;
    stream << cell.velocity;
    stream << cell.obstacle;
    stream << cell.river;
    stream << cell.waterSource;
    stream << cell.riverCapacity;
    return stream;
}

QDataStream& operator>>(QDataStream& stream, Cell& cell) {
    stream >> cell.terrainHeight;
    stream >> cell.waterDepth;
    stream >> cell.velocity;
    stream >> cell.obstacle;
    stream >> cell.river;
    stream >> cell.waterSource;
    stream >> cell.riverCapacity;
    return stream;
}


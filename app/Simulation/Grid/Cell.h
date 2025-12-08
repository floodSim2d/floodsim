#ifndef FLOODSIM_CELL_H
#define FLOODSIM_CELL_H

#include <QVector2D>
#include <algorithm>
#include <QDataStream>
#include <cstdint>

enum CellType : std::uint8_t {
    EMPTY,
    LAND,
    WATER,
    OBSTACLE,
};

class Cell {
public:
    explicit Cell(float height = 0.0f, float water_depth = 0.0f, bool is_obstacle = false, bool is_river = false,
                  bool is_water_source = false, float river_capacity = 3.0f);

    float getTerrainHeight() const { return terrainHeight; }
    void setTerrainHeight(float height) { terrainHeight = std::max(0.0f, height); }

    float getWaterDepth() const { return waterDepth; }
    void setWaterDepth(float depth) { waterDepth = std::max(0.0f, depth); }
    float getTotalHeight() const { return terrainHeight + waterDepth; }

    QVector2D getVelocity() const { return velocity; }
    void setVelocity(const QVector2D& vel) { velocity = vel; }

    CellType getType() const;
    void setType(CellType type);

    bool isObstacle() const { return obstacle; }
    void setObstacle(bool value) { obstacle = value; }

    bool isRiver() const { return river; }
    void setRiver(bool value) { river = value; }

    bool isWaterSource() const { return waterSource; }
    void setWaterSource(bool value) { waterSource = value; }

    // River channel capacity (max water depth before overflow)
    // Note: riverCapacity is the LIMIT, waterDepth is the CURRENT amount
    // When waterDepth > riverCapacity, the river overflows
    float getRiverCapacity() const { return riverCapacity; }
    void setRiverCapacity(float capacity) { riverCapacity = std::max(0.0f, capacity); }

    bool canFlowThrough() const { return !obstacle; }
    float getFlowCapacity() const;
    void addWater(float amount) { waterDepth = std::max(0.0f, waterDepth + amount); }
    void removeWater(float amount) { waterDepth = std::max(0.0f, waterDepth - amount); }

    void reset();
    void resetWater();

    // stream operators for serialization
    friend QDataStream& operator<<(QDataStream& stream, const Cell& cell);
    friend QDataStream& operator>>(QDataStream& stream, Cell& cell);

private:
    float terrainHeight;

    float waterDepth;
    QVector2D velocity;  // x, y components for 2D flow

    bool obstacle;
    bool river;
    bool waterSource;
    float riverCapacity;
};

#endif  // FLOODSIM_CELL_H

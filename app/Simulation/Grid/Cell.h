#ifndef FLOODSIM_CELL_H
#define FLOODSIM_CELL_H

#include <QVector2D>
#include <algorithm>

enum CellType : std::uint8_t {
    EMPTY,
    LAND,
    RIVER,
    WATER_SOURCE,
    RAIN,
    OBSTACLE,
};

class Cell {
public:
    explicit Cell(float height = 0.0F, float water_depth = 0.0F, bool is_obstacle = false, bool is_river = false,
                  bool is_water_source = false, float river_capacity = 3.0F, float source_strength = 1.0F);

    float getTerrainHeight() const { return terrainHeight; }
    void setTerrainHeight(float height);

    float getWaterDepth() const { return waterDepth; }
    void setWaterDepth(float depth) { waterDepth = std::max(0.0f, depth); }
    float getTotalHeight() const { return terrainHeight + waterDepth; }

    QVector2D getVelocity() const { return velocity; }
    void setVelocity(const QVector2D& vel) { velocity = vel; }

    CellType getType() const;
    void setType(CellType type);

    // river channel capacity (max water depth before overflow)
    // riverCapacity is the LIMIT, waterDepth is the CURRENT amount
    // waterDepth > riverCapacity -> the river overflows
    float getRiverCapacity() const { return riverCapacity; }
    void setRiverCapacity(float capacity) { riverCapacity = std::max(0.0f, capacity); }

    // water source properties
    bool isWaterSource() const { return waterSource; }
    float getSourceStrength() const { return sourceStrength; }
    void setSourceStrength(float strength) { sourceStrength = std::max(0.0f, strength); }

    // Rain area properties
    bool isRainArea() const { return rainArea; }
    float getRainIntensity() const { return rainIntensity; }
    void setRainIntensity(float intensity) { rainIntensity = std::max(0.0f, intensity); }

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
    bool rainArea;
    float riverCapacity;
    float sourceStrength;  // minimum water maintained by water source
    float rainIntensity;   // water added per time step in rain areas
};

#endif  // FLOODSIM_CELL_H

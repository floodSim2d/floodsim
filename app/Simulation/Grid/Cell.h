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

    [[nodiscard]] auto getTerrainHeight() const -> float { return terrainHeight; }
    void setTerrainHeight(float height);

    [[nodiscard]] auto getWaterDepth() const -> float { return waterDepth; }
    void setWaterDepth(const float depth) { waterDepth = std::max(0.0F, depth); }
    [[nodiscard]] auto getTotalHeight() const -> float { return terrainHeight + waterDepth; }

    [[nodiscard]] auto getVelocity() const -> QVector2D { return velocity; }
    void setVelocity(const QVector2D& vel) { velocity = vel; }

    [[nodiscard]] auto getType() const -> CellType;
    void setType(CellType type);

    // river channel capacity (max water depth before overflow)
    // riverCapacity is the LIMIT, waterDepth is the CURRENT amount
    // waterDepth > riverCapacity -> the river overflows
    [[nodiscard]] auto getRiverCapacity() const -> float { return riverCapacity; }
    void setRiverCapacity(float capacity) { riverCapacity = std::max(0.0F, capacity); }

    // water source properties
    [[nodiscard]] auto isWaterSource() const -> bool { return waterSource; }
    [[nodiscard]] auto getSourceStrength() const -> float { return sourceStrength; }
    void setSourceStrength(const float strength) { sourceStrength = std::max(0.0F, strength); }

    // Rain area properties
    [[nodiscard]] auto isRainArea() const -> bool { return rainArea; }
    [[nodiscard]] auto getRainIntensity() const -> float { return rainIntensity; }
    void setRainIntensity(const float intensity) { rainIntensity = std::max(0.0F, intensity); }

    [[nodiscard]] auto canFlowThrough() const -> bool { return !obstacle; }
    void addWater(const float amount) { waterDepth = std::max(0.0F, waterDepth + amount); }
    void removeWater(const float amount) { waterDepth = std::max(0.0F, waterDepth - amount); }

    void reset();
    void resetWater();

    // stream operators for serialization
    friend auto operator<<(QDataStream& stream, const Cell& cell) -> QDataStream&;
    friend auto operator>>(QDataStream& stream, Cell& cell) -> QDataStream&;

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

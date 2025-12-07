#ifndef FLOODSIM_PAINTTOOL_H
#define FLOODSIM_PAINTTOOL_H

#include <cstdint>

class Cell;
class Grid;

enum class ToolType : std::uint8_t {
    None,
    Terrain,
    Obstacle,
    River,
    WaterSource,
    Eraser
};

class PaintTool {
public:
    PaintTool();

    void setToolType(ToolType type);
    [[nodiscard]] ToolType getToolType() const { return currentTool; }

    void setBrushSize(int size);
    [[nodiscard]] int getBrushSize() const { return brushSize; }

    // Apply tool to a single cell or area based on brush size
    void applyTool(Grid* grid, int centerX, int centerY);

private:
    void applySingleCell(Cell* cell);

    ToolType currentTool;
    int brushSize;  // Radius of the brush (1 = single cell, 2 = 3x3, 3 = 5x5, etc.)
};

#endif  // FLOODSIM_PAINTTOOL_H


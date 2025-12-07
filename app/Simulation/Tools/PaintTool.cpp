#include "PaintTool.h"

#include "../Grid/Cell.h"
#include "../Grid/Grid.h"
#include <algorithm>

PaintTool::PaintTool()
    : currentTool(ToolType::None),
      brushSize(1) {
}

void PaintTool::setToolType(ToolType type) {
    currentTool = type;
}

void PaintTool::setBrushSize(int size) {
    brushSize = std::max(1, std::min(10, size));  // Clamp between 1 and 10
}

void PaintTool::applyTool(Grid* grid, int centerX, int centerY) {
    if (!grid || currentTool == ToolType::None) {
        return;
    }

    // Apply tool to all cells within brush radius
    for (int dy = -brushSize + 1; dy < brushSize; ++dy) {
        for (int dx = -brushSize + 1; dx < brushSize; ++dx) {
            const int targetX = centerX + dx;
            const int targetY = centerY + dy;

            // Check if within circular brush area
            const float distSquared = static_cast<float>(dx * dx + dy * dy);
            const float radiusSquared = static_cast<float>(brushSize * brushSize);

            if (distSquared < radiusSquared) {
                Cell* cell = grid->getCell(targetX, targetY);
                if (cell != nullptr) {
                    applySingleCell(cell);
                }
            }
        }
    }

    // Update the texture after painting
    grid->updateHeightTexture();
}

void PaintTool::applySingleCell(Cell* cell) {
    if (!cell) {
        return;
    }

    switch (currentTool) {
        case ToolType::Terrain:
            // Increase terrain height
            cell->setTerrainHeight(cell->getTerrainHeight() + 0.5F);
            break;

        case ToolType::Obstacle:
            // Set as obstacle
            cell->setObstacle(true);
            break;

        case ToolType::River:
            // Set as river with some water
            cell->setRiver(true);
            cell->setWaterDepth(1.0F);
            cell->setTerrainHeight(std::max(0.5F, cell->getTerrainHeight() - 0.5F));
            break;

        case ToolType::WaterSource:
            // Set as water source
            cell->setWaterSource(true);
            cell->setWaterDepth(2.0F);
            break;

        case ToolType::Eraser:
            // Reset cell to default
            *cell = Cell();
            break;

        case ToolType::None:
            // Do nothing
            break;
    }
}


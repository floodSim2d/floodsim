#include "PaintTool.h"

#include "../Grid/Cell.h"
#include "../Grid/Grid.h"
#include <algorithm>

#include "../../Renderer/OpenGLRenderer.h"

PaintTool::PaintTool(QObject* parent)
    : QObject(parent),
      currentTool(ToolType::Camera),
      brushSize(1),
      paintTimer(new QTimer(this)),
      currentGrid(nullptr),
      currentPaintGridX(-1),
      currentPaintGridY(-1),
      isContinuousPainting(false) {

    // Setup paint timer for continuous painting
    connect(paintTimer, &QTimer:: timeout, this, &PaintTool::applyPaintAtCurrentPosition);
    paintTimer->setInterval(50);  // Apply paint every 50ms (20 times per second)
}

void PaintTool::setToolType(ToolType type) {
    currentTool = type;
}

void PaintTool::setBrushSize(const int size) {
    brushSize = std::max(1, std::min(10, size));  // Clamp between 1 and 10
}

void PaintTool::applyTool(Grid* grid, const int centerX, const int centerY) const {
    if (grid == nullptr || currentTool == ToolType:: Camera) {
        return;
    }

    // Apply tool to all cells within brush radius
    for (int dy = -brushSize + 1; dy < brushSize; ++dy) {
        for (int dx = -brushSize + 1; dx < brushSize; ++dx) {
            const int targetX = centerX + dx;
            const int targetY = centerY + dy;

            // Check if within circular brush area
            const auto distSquared = static_cast<float>(dx * dx + dy * dy);
            const auto radiusSquared = static_cast<float>(brushSize * brushSize);

            if (distSquared < radiusSquared) {
                Cell* cell = grid->getCell(targetX, targetY);
                if (cell != nullptr) {
                    applySingleCell(cell);

                    //Capture states
                    const float prevTerrain = cell->getTerrainHeight();
                    const float prevWater = cell->getWaterDepth();
                    const auto prevType = cell->getType();

                    const float newTerrain = cell->getTerrainHeight();
                    const float newWater = cell->getWaterDepth();
                    const auto newType = cell->getType();

                    // If anything changed, log it
                    if (prevTerrain != newTerrain || prevWater != newWater || prevType != newType) {
                        qDebug().nospace() << "Map change by tool="
                                           << static_cast<int>(currentTool)
                                           << " at (" << targetX << "," << targetY << "): "
                                           << "type " << static_cast<int>(prevType) << "->" << static_cast<int>(newType)
                                           << ", terrain " << prevTerrain << "->" << newTerrain
                                           << ", water " << prevWater << "->" << newWater;
                    }
                }
            }
        }
    }

    // Update the texture after painting
    grid->updateHeightTexture();
}

void PaintTool::applySingleCell(Cell* cell) const {
    if (cell == nullptr || currentGrid == nullptr) {
        return;
    }

    switch (currentTool) {
        case ToolType::Terrain:
            // POPRAWIONE - Teren nadpisuje wszystko
            cell->setType(LAND);
            cell->setTerrainHeight(std::min(cell->getTerrainHeight() + 0.5F, CAMERA_MAX_HEIGHT));
            // Usuń wodę gdy budujesz teren
            cell->setWaterDepth(0.0F);
            cell->setSourceStrength(0.0F);
            cell->setRainIntensity(0.0F);
            break;

        case ToolType:: Obstacle:
            if (cell->getType() != OBSTACLE) {
                cell->setType(OBSTACLE);
            }
            break;

        case ToolType::River:
        {
            // POPRAWIONE - Rzeka tworzy wgłębienie z wodą
            cell->setType(RIVER);

            // Zawsze obniżaj teren tworząc wgłębienie
            const float newHeight = cell->getTerrainHeight() - 0.5F;
            if (newHeight >= -1.0F * currentGrid->getMaxDepth()) {
                cell->setTerrainHeight(newHeight);
            }

            // Dodaj wodę do rzeki
            if (cell->getWaterDepth() < cell->getRiverCapacity()) {
                cell->setWaterDepth(std::min(cell->getWaterDepth() + 0.5F, cell->getRiverCapacity()));
            }
            break;
        }

        case ToolType::WaterSource:
            if (cell->getType() != WATER_SOURCE) {
                cell->setType(WATER_SOURCE);
            }
            cell->setSourceStrength(cell->getSourceStrength() + 0.2F);
            cell->setWaterDepth(std::max(cell->getWaterDepth(), cell->getSourceStrength()));
            break;

        case ToolType::Eraser:
            *cell = Cell();
            break;

        case ToolType::Camera:
            // Do nothing
            break;
    }
}

void PaintTool::startContinuousPainting(Grid* grid, int gridX, int gridY) {
    if (grid == nullptr || currentTool == ToolType::Camera) {
        return;
    }

    currentGrid = grid;
    currentPaintGridX = gridX;
    currentPaintGridY = gridY;
    isContinuousPainting = true;

    qDebug().nospace() << "Paint: START tool=" << static_cast<int>(currentTool)
                       << " at (" << gridX << "," << gridY << ") brush=" << brushSize;

    // Apply immediately on start
    applyTool(grid, gridX, gridY);
    emit paintApplied();

    // Start timer for continuous painting
    paintTimer->start();
}

void PaintTool::updatePaintPosition(int gridX, int gridY) {
    currentPaintGridX = gridX;
    currentPaintGridY = gridY;

    // Apply immediately when position updates
    if (isContinuousPainting && currentGrid != nullptr) {
        applyTool(currentGrid, gridX, gridY);
        emit paintApplied();
    }
}

void PaintTool::stopContinuousPainting() {
    isContinuousPainting = false;
    paintTimer->stop();
    currentGrid = nullptr;

    qDebug() << "Paint: STOP";
}

void PaintTool::applyPaintAtCurrentPosition() {
    if (!isContinuousPainting || currentGrid == nullptr) {
        return;
    }

    if (currentGrid->isValidPosition(currentPaintGridX, currentPaintGridY)) {
        applyTool(currentGrid, currentPaintGridX, currentPaintGridY);
        emit paintApplied();
    }
}
#include "PaintTool.h"

#include "../Grid/Cell.h"
#include "../Grid/Grid.h"
#include <algorithm>

#include "../../Utils/Logger.h"
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

    connect(paintTimer, &QTimer:: timeout, this, &PaintTool::applyPaintAtCurrentPosition);
    paintTimer->setInterval(50);  // apply paint every 50ms (20 times per second)
}

void PaintTool::setToolType(ToolType type) {
    currentTool = type;
}

void PaintTool::setBrushSize(const int size) {
    brushSize = std::max(1, std::min(10, size));
}

void PaintTool::applyTool(Grid* grid, const int centerX, const int centerY, const bool isAlternateMode) const {
    if (grid == nullptr || currentTool == ToolType:: Camera) {
        return;
    }

    for (int dy = -brushSize + 1; dy < brushSize; ++dy) {
        for (int dx = -brushSize + 1; dx < brushSize; ++dx) {
            const int targetX = centerX + dx;
            const int targetY = centerY + dy;

            const auto distSquared = static_cast<float>(dx * dx + dy * dy);
            const auto radiusSquared = static_cast<float>(brushSize * brushSize);

            if (distSquared < radiusSquared) {
                Cell* cell = grid->getCell(targetX, targetY);
                if (cell != nullptr) {
                    applySingleCell(cell, isAlternateMode);

                    const float prevTerrain = cell->getTerrainHeight();
                    const float prevWater = cell->getWaterDepth();
                    const auto prevType = cell->getType();

                    const float newTerrain = cell->getTerrainHeight();
                    const float newWater = cell->getWaterDepth();
                    const auto newType = cell->getType();

                    if (prevTerrain != newTerrain || prevWater != newWater || prevType != newType) {
                        LOG(QString("Map change by tool=%1 at (%2,%3): type %4->%5, terrain %6->%7, water %8->%9")
                            .arg(static_cast<int>(currentTool))
                            .arg(targetX).arg(targetY)
                            .arg(static_cast<int>(prevType)).arg(static_cast<int>(newType))
                            .arg(prevTerrain).arg(newTerrain)
                            .arg(prevWater).arg(newWater));
                    }
                }
            }
        }
    }

    grid->updateHeightTexture();
}

void PaintTool::applySingleCell(Cell* cell, bool isAlternateMode) const {
    if (cell == nullptr || currentGrid == nullptr) {
        return;
    }

    switch (currentTool) {
        case ToolType::Terrain:
            if (isAlternateMode) {
                // right-click: lower terrain
                cell->setTerrainHeight(std::max(cell->getTerrainHeight() - 0.5F, -1.0F * currentGrid->getMaxDepth()));
            } else {
                // left-click: raise terrain (original behavior)
                cell->setType(LAND);
                cell->setTerrainHeight(std::min(cell->getTerrainHeight() + 0.5F, CAMERA_MAX_HEIGHT));
                cell->setWaterDepth(0.0F);
                cell->setSourceStrength(0.0F);
                cell->setRainIntensity(0.0F);
            }
            break;

        case ToolType::Obstacle:
            if (isAlternateMode) {
                // right-click: remove obstacle (convert to land)
                if (cell->getType() == OBSTACLE) {
                    cell->setType(LAND);
                }
            } else {
                // left-click: create obstacle (original behavior)
                if (cell->getType() != OBSTACLE) {
                    cell->setType(OBSTACLE);
                }
            }
            break;

        case ToolType::River:
        {
            if (isAlternateMode) {
                // right-click: raise terrain and remove water
                const float newHeight = cell->getTerrainHeight() + 0.5F;
                if (newHeight <= CAMERA_MAX_HEIGHT) {
                    cell->setTerrainHeight(newHeight);
                }
                // reduce water
                cell->setWaterDepth(std::max(0.0F, cell->getWaterDepth() - 0.5F));
                // if no water left, convert to land
                if (cell->getWaterDepth() < 0.01F && cell->getType() == RIVER) {
                    cell->setType(LAND);
                }
            } else {
                // left-click: create river (original behavior)
                cell->setType(RIVER);

                const float newHeight = cell->getTerrainHeight() - 0.5F;
                if (newHeight >= -1.0F * currentGrid->getMaxDepth()) {
                    cell->setTerrainHeight(newHeight);
                }

                if (cell->getWaterDepth() < cell->getRiverCapacity()) {
                    cell->setWaterDepth(std::min(cell->getWaterDepth() + 0.5F, cell->getRiverCapacity()));
                }
            }
            break;
        }

        case ToolType::WaterSource:
            if (isAlternateMode) {
                // right-click: raise terrain and decrease water
                const float newHeight = cell->getTerrainHeight() + 0.25F;
                if (newHeight <= CAMERA_MAX_HEIGHT) {
                    cell->setTerrainHeight(newHeight);
                }

                // decrease source strength
                float newStrength = cell->getSourceStrength() - 0.2F;
                if (newStrength <= 0.0F) {
                    // remove source entirely
                    cell->setSourceStrength(0.0F);
                    cell->setWaterDepth(0.0F);
                    if (cell->getType() == WATER_SOURCE) {
                        cell->setType(LAND);
                    }
                } else {
                    cell->setSourceStrength(newStrength);
                    cell->setWaterDepth(newStrength);
                }
            } else {
                // left-click: create depression and fill with water (like a spring/lake)
                if (cell->getType() != WATER_SOURCE) {
                    cell->setType(WATER_SOURCE);
                    // lower terrain to create depression
                    const float newHeight = cell->getTerrainHeight() - 0.25F;
                    if (newHeight >= -1.0F * currentGrid->getMaxDepth()) {
                        cell->setTerrainHeight(newHeight);
                    }
                    // start with moderate strength
                    cell->setSourceStrength(0.5F);
                    cell->setWaterDepth(0.5F);
                } else {
                    // deepen existing source and increase strength (capped at 3.0)
                    const float newHeight = cell->getTerrainHeight() - 0.25F;
                    if (newHeight >= -1.0F * currentGrid->getMaxDepth()) {
                        cell->setTerrainHeight(newHeight);
                    }

                    float newStrength = std::min(cell->getSourceStrength() + 0.2F, 3.0F);
                    cell->setSourceStrength(newStrength);
                    cell->setWaterDepth(newStrength);
                }
            }
            break;

        case ToolType::Eraser:
            // eraser works the same for both clicks
            *cell = Cell();
            break;

        case ToolType::Camera:
            break;
    }
}

void PaintTool::startContinuousPainting(Grid* grid, const int gridX, const int gridY, const bool isAlternateMode) {
    if (grid == nullptr || currentTool == ToolType::Camera) {
        return;
    }

    currentGrid = grid;
    currentPaintGridX = gridX;
    currentPaintGridY = gridY;
    isContinuousPainting = true;
    isAlternateModeActive = isAlternateMode;

    LOG(QString("Paint: START tool=%1 at (%2,%3) brush=%4 alternate=%5")
        .arg(static_cast<int>(currentTool))
        .arg(gridX).arg(gridY)
        .arg(brushSize)
        .arg(isAlternateMode ? "true" : "false"));

    applyTool(grid, gridX, gridY, isAlternateMode);
    emit paintApplied();

    paintTimer->start();
}

void PaintTool::updatePaintPosition(int gridX, int gridY) {
    currentPaintGridX = gridX;
    currentPaintGridY = gridY;

    if (isContinuousPainting && currentGrid != nullptr) {
        applyTool(currentGrid, gridX, gridY, isAlternateModeActive);
        emit paintApplied();
    }
}

void PaintTool::stopContinuousPainting() {
    isContinuousPainting = false;
    paintTimer->stop();
    currentGrid = nullptr;

    LOG("Paint: STOP");
}

void PaintTool::applyPaintAtCurrentPosition() {
    if (!isContinuousPainting || currentGrid == nullptr) {
        return;
    }

    if (currentGrid->isValidPosition(currentPaintGridX, currentPaintGridY)) {
        applyTool(currentGrid, currentPaintGridX, currentPaintGridY, isAlternateModeActive);
        emit paintApplied();
    }
}
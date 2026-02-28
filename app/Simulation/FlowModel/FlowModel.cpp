#include "FlowModel.h"

#include <algorithm>

#include "../Grid/Grid.h"

FlowModel::FlowModel(Grid* grid, QObject* parent)
    : QObject(parent),
      grid(grid),
      timer(new QTimer(this)),
      playing(false),
      dt(0.016f),              // ~60 FPS default
      flowCoefficient(0.5f),   // Moderate flow speed
      dampingFactor(0.999f),   // 0.1% energy loss per step so water doesn't oscillate indefinitely
      updateInterval(16),      // ~60 FPS
      globalRainEnabled(false),
      globalRainIntensity(0.0f),
      infiltrationRate(0.0f) {

    connect(timer, &QTimer::timeout, this, &FlowModel::update);

    // Initialize flow buffer
    if (grid != nullptr) {
        flowBuffer.resize(grid->getWidth() * grid->getHeight());
    }
}

void FlowModel::play() {
    if (!playing && grid != nullptr) {
        playing = true;
        timer->start(updateInterval);
        emit simulationStarted();
    }
}

void FlowModel::pause() {
    if (playing) {
        playing = false;
        timer->stop();
        emit simulationPaused();
    }
}

void FlowModel::stop() {
    if (playing || timer->isActive()) {
        playing = false;
        timer->stop();
        emit simulationStopped();
    }
}

void FlowModel::step() {
    if (grid != nullptr && !playing) {
        computeFlowStep();
        emit stepCompleted();
    }
}

void FlowModel::setUpdateInterval(int interval) {
    updateInterval = interval;
    if (timer->isActive()) {
        timer->setInterval(updateInterval);
    }
}

void FlowModel::setGlobalRainEnabled(bool enabled) {
    globalRainEnabled = enabled;
}

void FlowModel::setGlobalRainIntensity(float intensity) {
    globalRainIntensity = intensity;
}

void FlowModel::update() {
    if (grid != nullptr && playing) {
        computeFlowStep();
        emit stepCompleted();
    }
}

void FlowModel::computeFlowStep() {
    const unsigned int width = grid->getWidth();
    const unsigned int height = grid->getHeight();
    const float cellArea = grid->getCellSize() * grid->getCellSize();

    std::ranges::fill(flowBuffer, FlowData{.netFlow=0.0F, .totalOutflow=0.0F});

    // calculate flows between all neighboring cells
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Cell* cell = grid->getCell(x, y);
            if (cell == nullptr || !cell->canFlowThrough()) {
                continue;
            }

            const int idx = y * width + x;
            const float h_total_i = cell->getTotalHeight();

            // check all 4 neighbors (up, down, left, right)
            const int dx[] = {0, 0, -1, 1};
            const int dy[] = {-1, 1, 0, 0};

            for (int dir = 0; dir < 4; ++dir) {
                const int nx = x + dx[dir];
                const int ny = y + dy[dir];

                if (!grid->isValidPosition(nx, ny)) {
                    continue;
                }

                const Cell* neighbor = grid->getCell(nx, ny);
                if (neighbor == nullptr || !neighbor->canFlowThrough()) {
                    continue;
                }

                const float h_total_j = neighbor->getTotalHeight();
                const float heightDiff = h_total_i - h_total_j;

                if (heightDiff > 0.0f) {
                    // water flows from current cell to neighbor
                    float flow = calculateOutflow(x, y, nx, ny);
                    flow = std::min(flow, cell->getWaterDepth()); // can't flow more than available water

                    const int nidx = ny * width + nx;

                    flowBuffer[idx].netFlow -= flow;
                    flowBuffer[idx].totalOutflow += flow;
                    flowBuffer[nidx].netFlow += flow;
                }
            }
        }
    }

    applyWaterSources();

    applyRainfall();

    applyInfiltration();

    // apply flows to cells and update water depths
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Cell* cell = grid->getCell(x, y);
            if (cell == nullptr) {
                continue;
            }

            const int idx = y * width + x;
            const float flowChange = (flowBuffer[idx].netFlow * dt) / cellArea;

            float newWaterDepth = cell->getWaterDepth() + flowChange;

            // apply damping based on outflow intensity to prevent oscillations
            if (flowBuffer[idx].totalOutflow > 0.0F && newWaterDepth > 0.01F) {
                // Apply damping as a tiny friction, scaled by flow intensity
                const float flowIntensity = std::min(flowBuffer[idx].totalOutflow / cell->getWaterDepth(), 1.0F);
                newWaterDepth *= 1.0F - (1.0F - dampingFactor) * flowIntensity;
            }

            // handle river overflow - excess water stays but doesn't count toward capacity check
            // the water can overflow onto neighboring land cells
            cell->setWaterDepth(std::max(0.0F, newWaterDepth));
        }
    }

    updateVelocities();

    grid->updateHeightTexture();
}

auto FlowModel::calculateOutflow(int x, int y, int nx, int ny) const -> float {
    const Cell* cell = grid->getCell(x, y);
    const Cell* neighbor = grid->getCell(nx, ny);

    if (cell == nullptr || neighbor == nullptr) {
        return 0.0F;
    }

    const float h_total_i = cell->getTotalHeight();
    const float h_total_j = neighbor->getTotalHeight();
    const float heightDiff = std::max(0.0F, h_total_i - h_total_j);

    // basic flow equation: Q = k * height_diff
    float flow = flowCoefficient * heightDiff;

    // consider neighbor's capacity constraints
    if (neighbor->getType() == RIVER) {
        const float availableCapacity = neighbor->getRiverCapacity() - neighbor->getWaterDepth();
        flow = std::min(flow, std::max(0.0F, availableCapacity));
    }

    const float maxFlow = cell->getWaterDepth();
    flow = std::min(flow, maxFlow);

    return flow;
}

/*
 * Applies water sources - maintains constant water level at source cells
 * Water sources represent springs, lakes, or other permanent water bodies
 * They maintain a minimum water level regardless of outflow
 */
void FlowModel::applyWaterSources() {
    const auto width = grid->getWidth();
    const auto height = grid->getHeight();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Cell* cell = grid->getCell(x, y);
            if (cell != nullptr && cell->isWaterSource()) {
                // water sources maintain a minimum water level
                // this simulates a spring or lake that has a constant inflow
                const float minSourceLevel = cell->getSourceStrength();
                if (cell->getWaterDepth() < minSourceLevel) {
                    // add water up to the source strength
                    const float deficit = minSourceLevel - cell->getWaterDepth();
                    const int idx = y * width + x;
                    flowBuffer[idx].netFlow += deficit / dt;
                }
            }
        }
    }
}

/*
 * Applies rainfall - adds water to the entire grid if global rain is enabled
 */
void FlowModel::applyRainfall() {
    if (!globalRainEnabled || globalRainIntensity <= 0.0001f) {
        return;
    }

    const auto width = grid->getWidth();
    const auto height = grid->getHeight();
    const float cellArea = grid->getCellSize() * grid->getCellSize();

    // Calculate volume of water to add per cell: Intensity (depth/sec) * Area
    const float flowToAdd = globalRainIntensity * cellArea;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Cell* cell = grid->getCell(x, y);
            // Rain falls on everything except obstacles (assuming obstacles are solid walls)
            if (cell != nullptr && cell->canFlowThrough()) {
                const int idx = y * width + x;
                flowBuffer[idx].netFlow += flowToAdd;
            }
        }
    }
}

/*
 * Applies infiltration - water is absorbed by the ground on non-river, non-obstacle cells.
 * Only applies to LAND/EMPTY/RAIN cells (anything that is not a RIVER or WATER_SOURCE).
 */
void FlowModel::applyInfiltration() {
    if (infiltrationRate <= 0.0001f) {
        return;
    }

    const auto width = grid->getWidth();
    const auto height = grid->getHeight();
    const float cellArea = grid->getCellSize() * grid->getCellSize();

    // Amount of water depth absorbed per step
    const float absorb = infiltrationRate * dt;
    const float flowToRemove = absorb * cellArea;

    for (int y = 0; y < static_cast<int>(height); ++y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            const Cell* cell = grid->getCell(x, y);
            if (cell == nullptr) {
                continue;
            }
            // Only absorb on non-river, non-water-source, non-obstacle cells
            const CellType type = cell->getType();
            if (type == RIVER || type == WATER_SOURCE || type == OBSTACLE) {
                continue;
            }
            if (cell->getWaterDepth() <= 0.0f) {
                continue;
            }
            const int idx = y * width + x;
            flowBuffer[idx].netFlow -= flowToRemove;
        }
    }
}

/*
 * updates cell velocities based on water surface gradients
 */
void FlowModel::updateVelocities() const {
    const unsigned int width = grid->getWidth();
    const unsigned int height = grid->getHeight();
    const float cellSize = grid->getCellSize();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            updateCellVelocity(x, y, cellSize);
        }
    }
}

void FlowModel::updateCellVelocity(int x, int y, const float cellSize) const {
    Cell* cell = grid->getCell(x, y);
    if (cell == nullptr) {
        return;
    }

    if (cell->getWaterDepth() < 0.01F) {
        cell->setVelocity(QVector2D(0.0F, 0.0F));
        return;
    }

    const float gradX = calculateGradientX(x, y, cellSize);
    const float gradY = calculateGradientY(x, y, cellSize);

    // velocity proportional to gradient (simplified momentum equation)
    const float velocityScale = 0.5F; // TODO: adjust for realistic velocities
    QVector2D velocity(gradX * velocityScale, gradY * velocityScale);


    velocity *= dampingFactor;

    cell->setVelocity(velocity);
}

auto FlowModel::calculateGradientX(const int x, const int y, const float cellSize) const -> float {
    if (!grid->isValidPosition(x - 1, y) || !grid->isValidPosition(x + 1, y)) {
        return 0.0F;
    }

    const Cell* west = grid->getCell(x - 1, y);
    const Cell* east = grid->getCell(x + 1, y);

    if (west == nullptr || east == nullptr) {
        return 0.0F;
    }

    return (west->getTotalHeight() - east->getTotalHeight()) / (2.0f * cellSize);
}

auto FlowModel::calculateGradientY(const int x, const int y, const float cellSize) const -> float {
    if (!grid->isValidPosition(x, y - 1) || !grid->isValidPosition(x, y + 1)) {
        return 0.0F;
    }

    const Cell* south = grid->getCell(x, y - 1);
    const Cell* north = grid->getCell(x, y + 1);

    if (south == nullptr || north == nullptr) {
        return 0.0F;
    }

    return (south->getTotalHeight() - north->getTotalHeight()) / (2.0f * cellSize);
}
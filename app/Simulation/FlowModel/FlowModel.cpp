#include "FlowModel.h"

#include <algorithm>
#include <cmath>

#include "../Grid/Grid.h"

constexpr int FlowModel::DX[NUM_DIRS];
constexpr int FlowModel::DY[NUM_DIRS];

FlowModel::FlowModel(Grid* grid, QObject* parent)
    : QObject(parent),
      grid(grid),
      timer(new QTimer(this)),
      playing(false),
      dt(0.016F),
      updateInterval(16),
      pipeFriction(0.5F),
      globalRainEnabled(false),
      globalRainIntensity(0.0F),
      infiltrationRate(0.0F) {

    connect(timer, &QTimer::timeout, this, &FlowModel::update);

    if (grid != nullptr) {
        pipeBuffer.resize(grid->getWidth() * grid->getHeight());
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

        for (auto& pipe : pipeBuffer) {
            pipe.flux.fill(0.0F);
        }

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
    const auto g_width = grid->getWidth();
    const auto g_height = grid->getHeight();
    const float pipe_length = grid->getCellSize();
    const float A = pipe_length;                                  // cross-section area
    const float cellArea = pipe_length * pipe_length;

    // Friction damping factor: exponential decay per step
    // pipeFriction=0 → no damping, pipeFriction=1 → moderate, higher = more
    const float dampFactor = std::exp(-pipeFriction * dt);

    for (auto y = 0; y < g_height; ++y) {
        for (auto x = 0; x < g_width; ++x) {
            const int idx = y * g_width + x;
            const Cell* cell = grid->getCell(x, y);

            if (cell == nullptr || !cell->canFlowThrough()) {
                pipeBuffer[idx].flux.fill(0.0F);
                continue;
            }

            const float h_total = cell->getTotalHeight();
            float totalOutflux = 0.0F;

            for (int d = 0; d < NUM_DIRS; ++d) {
                const int nx = x + DX[d];
                const int ny = y + DY[d];

                if (!grid->isValidPosition(nx, ny)) {
                    pipeBuffer[idx].flux[d] = 0.0F;
                    continue;
                }

                const Cell* neighbor = grid->getCell(nx, ny);
                if (neighbor == nullptr || !neighbor->canFlowThrough()) {
                    pipeBuffer[idx].flux[d] = 0.0F;
                    continue;
                }

                const float h_total_n = neighbor->getTotalHeight();
                const float deltaH = h_total - h_total_n;

                // Pipe equation: Q = Q_old * damp + dt * A * g * Δh / L
                float newFlux = dampFactor * pipeBuffer[idx].flux[d]
                              + dt * A * GRAVITY * deltaH / pipe_length;

                // Flux can't be negative (negative flow is
                // handled by the neighbor's pipe in the opposite direction)
                newFlux = std::max(0.0F, newFlux);

                pipeBuffer[idx].flux[d] = newFlux;
                totalOutflux += newFlux;
            }

            // mass conservation: scale down if outflow > available water
            const float availableVolume = cell->getWaterDepth() * cellArea;
            const float outVolume = totalOutflux * dt;

            if (outVolume > availableVolume && outVolume > 1e-8F) {
                const float scale = availableVolume / outVolume;
                for (int d = 0; d < NUM_DIRS; ++d) {
                    pipeBuffer[idx].flux[d] *= scale;
                }
            }
        }
    }

    applyWaterSources();
    applyRainfall();
    applyInfiltration();

    // For each cell:
    //   ΔV = dt * (sum of inflow fluxes - sum of outflow fluxes) + source/rain deltas
    //   Δh = ΔV / cellArea
    // Inflow to cell (x,y) from direction d comes from neighbor's pipe in oppositeDir(d).

    for (int y = 0; y < g_height; ++y) {
        for (int x = 0; x < g_width; ++x) {
            Cell* cell = grid->getCell(x, y);
            if (cell == nullptr) continue;

            const int idx = y * g_width + x;

            float totalInflux  = 0.0F;
            float totalOutflux = 0.0F;

            for (int d = 0; d < NUM_DIRS; ++d) {
                // Outflow from this cell
                totalOutflux += pipeBuffer[idx].flux[d];

                // Inflow from neighbor in direction d
                const int nx = x + DX[d];
                const int ny = y + DY[d];
                if (grid->isValidPosition(nx, ny)) {
                    const int nidx = ny * g_width + nx;
                    totalInflux += pipeBuffer[nidx].flux[oppositeDir(d)];
                }
            }

            const float deltaVolume = dt * (totalInflux - totalOutflux);
            const float deltaDepth = deltaVolume / cellArea;

            float newDepth = cell->getWaterDepth() + deltaDepth;
            cell->setWaterDepth(std::max(0.0F, newDepth));
        }
    }

    // ─── Step 3: Derive velocity from flux differences ───────────────────
    updateVelocities();

    grid->updateHeightTexture();
}

void FlowModel::applyWaterSources() const {
    const auto width = grid->getWidth();
    const auto height = grid->getHeight();

    for (auto y = 0; y < height; ++y) {
        for (auto x = 0; x < width; ++x) {
            auto *cell = grid->getCell(x, y);
            if (cell != nullptr && cell->isWaterSource()) {
                const float minLevel = cell->getSourceStrength();
                if (cell->getWaterDepth() < minLevel) {
                    cell->setWaterDepth(minLevel);
                }
            }
        }
    }
}

// =============================================================================
// RAINFALL — add water depth directly
// =============================================================================
void FlowModel::applyRainfall() const {
    if (!globalRainEnabled || globalRainIntensity <= 0.0001F) {
        return;
    }

    const auto width = grid->getWidth();
    const auto height = grid->getHeight();

    // Rain adds depth/second directly: Δh = intensity * dt
    const float depthToAdd = globalRainIntensity * dt;

    for (auto y = 0; y < height; ++y) {
        for (auto x = 0; x < width; ++x) {
            Cell* cell = grid->getCell(x, y);
            if (cell != nullptr && cell->canFlowThrough()) {
                cell->addWater(depthToAdd);
            }
        }
    }
}

// =============================================================================
// INFILTRATION — remove water on non-river/source cells
// =============================================================================
void FlowModel::applyInfiltration() const {
    if (infiltrationRate <= 0.0001F) {
        return;
    }
    const auto width = grid->getWidth();
    const auto height = grid->getHeight();

    const float depthToRemove = infiltrationRate * dt;

    for (auto y = 0; y < height; ++y) {
        for (auto x = 0; x < width; ++x) {
            Cell* cell = grid->getCell(x, y);
            if (cell == nullptr) {
                continue;
            }

            const CellType type = cell->getType();
            if (type == RIVER || type == WATER_SOURCE || type == OBSTACLE) {
                continue;
            }
            if (cell->getWaterDepth() <= 0.0F) {
                continue;
            }

            cell->removeWater(depthToRemove);
        }
    }
}

// =============================================================================
// VELOCITY — derived from horizontal flux differences
// =============================================================================
void FlowModel::updateVelocities() const {
    const auto g_width = grid->getWidth();
    const auto g_height = grid->getHeight();
    const float L = grid->getCellSize();

    for (auto y = 0; y < g_height; ++y) {
        for (auto x = 0; x < g_width; ++x) {
            Cell* cell = grid->getCell(x, y);
            if (cell == nullptr) {
                continue;
            }

            if (cell->getWaterDepth() < 0.01F) {
                cell->setVelocity(QVector2D(0.0F, 0.0F));
                continue;
            }

            const int idx = y * g_width + x;

            float fluxL_in = 0.0F;
            float fluxR_in = 0.0F;
            float fluxU_in = 0.0F;
            float fluxD_in = 0.0F;

            if (grid->isValidPosition(x - 1, y)) {
                fluxL_in = pipeBuffer[(y * g_width) + (x - 1)].flux[DIR_RIGHT];
            }
            if (grid->isValidPosition(x + 1, y)) {
                fluxR_in = pipeBuffer[(y * g_width) + (x + 1)].flux[DIR_LEFT];
            }
            if (grid->isValidPosition(x, y - 1)) {
                fluxU_in = pipeBuffer[((y - 1) * g_width) + x].flux[DIR_DOWN];
            }
            if (grid->isValidPosition(x, y + 1)) {
                fluxD_in = pipeBuffer[((y + 1) * g_width) + x].flux[DIR_UP];
            }

            const float fluxL_out = pipeBuffer[idx].flux[DIR_LEFT];
            const float fluxR_out = pipeBuffer[idx].flux[DIR_RIGHT];
            const float fluxU_out = pipeBuffer[idx].flux[DIR_UP];
            const float fluxD_out = pipeBuffer[idx].flux[DIR_DOWN];

            // Net flux in X direction (positive = rightward)
            const float netFluxX = (fluxL_in + fluxR_out - fluxR_in - fluxL_out) * 0.5F;
            // Net flux in Y direction (positive = downward)
            const float netFluxY = (fluxU_in + fluxD_out - fluxD_in - fluxU_out) * 0.5F;

            // Velocity = flux / (L * waterDepth)
            const float depthClamped = std::max(cell->getWaterDepth(), 0.01F);
            const float vx = netFluxX / (L * depthClamped);
            const float vy = netFluxY / (L * depthClamped);

            cell->setVelocity(QVector2D(vx, vy));
        }
    }
}

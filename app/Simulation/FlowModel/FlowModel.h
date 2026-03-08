#ifndef FLOODSIM_FLOWMODEL_H
#define FLOODSIM_FLOWMODEL_H

#include <QObject>
#include <QTimer>
#include <array>

class Grid;

/**
 * @brief Pipe-model water flow simulation.
 *
 * Uses persistent pipe fluxes between neighboring cells (left/right/up/down).
 * Each step:
 *   1. Update pipe fluxes:  Q_new = max(0, Q_old + dt * A * g * Δh / L)
 *   2. Scale down if total outflow > available water (mass conservation)
 *   3. Update water depths from net flux
 *   4. Derive velocity from flux differences
 *
 * "Fast Hydraulic Erosion Simulation and Visualization on GPU"
 *            (Xing Mei, Philippe Decaudin, Bao-Gang Hu, 2007)
 */
class FlowModel : public QObject {
    Q_OBJECT

   public:
    explicit FlowModel(Grid* grid, QObject* parent = nullptr);
    ~FlowModel() override = default;

    // control functions
    void play();
    void pause();
    void stop();
    void step();

    [[nodiscard]] auto isPlaying() const -> bool { return playing; }
    [[nodiscard]] auto getTimeStep() const -> float { return dt; }
    void setTimeStep(float timeStep) { dt = timeStep; }
    [[nodiscard]] int getUpdateInterval() const { return updateInterval; }
    void setUpdateInterval(int interval);

    [[nodiscard]] float getPipeFriction() const { return pipeFriction; }
    void setPipeFriction(float friction) { pipeFriction = std::max(0.0F, friction); }

    void setGlobalRainEnabled(bool enabled);
    [[nodiscard]] bool isGlobalRainEnabled() const { return globalRainEnabled; }
    void setGlobalRainIntensity(float intensity);
    [[nodiscard]] float getGlobalRainIntensity() const { return globalRainIntensity; }

    void setInfiltrationRate(float rate) { infiltrationRate = rate; }
    [[nodiscard]] float getInfiltrationRate() const { return infiltrationRate; }

   signals:
    void simulationStarted();
    void simulationPaused();
    void simulationStopped();
    void stepCompleted();

   private slots:
    void update();

   private:
    void computeFlowStep();
    void applyWaterSources() const;
    void applyRainfall() const;
    void applyInfiltration() const;
    void updateVelocities() const;

    Grid* grid;
    QTimer* timer;

    // Simulation state
    bool playing;
    float dt;
    int updateInterval;

    // Physics constants
    static constexpr float GRAVITY = 9.81F;      // m/s²
    float pipeFriction;                            // friction coefficient for exponential damping

    bool globalRainEnabled;
    float globalRainIntensity;

    float infiltrationRate;
    static constexpr int DIR_LEFT  = 0;
    static constexpr int DIR_RIGHT = 1;
    static constexpr int DIR_UP    = 2;
    static constexpr int DIR_DOWN  = 3;
    static constexpr int NUM_DIRS  = 4;
    static constexpr int DX[NUM_DIRS] = {-1,  1,  0,  0};
    static constexpr int DY[NUM_DIRS] = { 0,  0, -1,  1};

    static constexpr auto oppositeDir(const int dir) -> int {
        // left<->right, up<->down
        return dir ^ 1;
    }

    /**
     * @brief Per-cell persistent pipe flux data.
     *
     * flux[d] = volume flow rate (m³/s) through pipe in direction d.
     * Positive = flowing OUT of this cell in direction d.
     */
    struct PipeData {
        std::array<float, NUM_DIRS> flux = {0.0F, 0.0F, 0.0F, 0.0F};
    };

    std::vector<PipeData> pipeBuffer;
};

#endif  // FLOODSIM_FLOWMODEL_H
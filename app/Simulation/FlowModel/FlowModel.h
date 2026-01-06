#ifndef FLOODSIM_FLOWMODEL_H
#define FLOODSIM_FLOWMODEL_H

#include <QObject>
#include <QTimer>

class Grid;

class FlowModel : public QObject {
    Q_OBJECT

   public:
    explicit FlowModel(Grid* grid, QObject* parent = nullptr);
    ~FlowModel() override = default;

    // Control functions
    void play();
    void pause();
    void stop();
    void step();

    // getters setters
    auto isPlaying() const -> bool { return playing; }
    float getTimeStep() const { return dt; }
    void setTimeStep(float timeStep) { dt = timeStep; }
    float getFlowCoefficient() const { return flowCoefficient; }
    void setFlowCoefficient(float k) { flowCoefficient = k; }
    float getDampingFactor() const { return dampingFactor; }
    void setDampingFactor(float damping) { dampingFactor = damping; }
    int getUpdateInterval() const { return updateInterval; }
    void setUpdateInterval(int interval);

    // Global Rain Control
    void setGlobalRainEnabled(bool enabled);
    bool isGlobalRainEnabled() const { return globalRainEnabled; }
    void setGlobalRainIntensity(float intensity);
    float getGlobalRainIntensity() const { return globalRainIntensity; }

   signals:
    void simulationStarted();
    void simulationPaused();
    void simulationStopped();
    void stepCompleted();

   private slots:
    void update();

   private:
    void computeFlowStep();
    float calculateOutflow(int x, int y, int nx, int ny) const;
    void applyWaterSources();
    void applyRainfall();
    void updateVelocities() const;
    void updateCellVelocity(int x, int y, float cellSize) const;
    float calculateGradientX(int x, int y, float cellSize) const;
    float calculateGradientY(int x, int y, float cellSize) const;

    Grid* grid;
    QTimer* timer;

    // simulation state
    bool playing;
    float dt;                  // time step
    float flowCoefficient;     // k in Q = k * max(0, h_total_i - h_total_j)
    float dampingFactor;       // energy loss during flow (0.0 - 1.0)
    int updateInterval;        // timer interval in milliseconds

    // Global rain state
    bool globalRainEnabled;
    float globalRainIntensity; // Water depth added per second

    // flow data for current step
    struct FlowData {
        float netFlow;         // net flow into cell
        float totalOutflow;    // flowing out
    };
    std::vector<FlowData> flowBuffer;
};

#endif  // FLOODSIM_FLOWMODEL_H
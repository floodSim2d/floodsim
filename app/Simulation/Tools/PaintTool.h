#ifndef FLOODSIM_PAINTTOOL_H
#define FLOODSIM_PAINTTOOL_H

#include <QTimer>
#include <QObject>

class Cell;
class Grid;

enum class ToolType : std::uint8_t {
    Camera,
    Terrain,
    Obstacle,
    River,
    WaterSource,
    Eraser
};

class PaintTool : public QObject {
    Q_OBJECT

public:
    explicit PaintTool(QObject* parent = nullptr);

    void setToolType(ToolType type);
    [[nodiscard]] ToolType getToolType() const { return currentTool; }

    void setBrushSize(int size);
    [[nodiscard]] int getBrushSize() const { return brushSize; }

    // Apply tool to a single cell or area based on brush size
    // isAlternateMode = true for right-click (opposite effect)
    void applyTool(Grid* grid, int centerX, int centerY, bool isAlternateMode = false) const;

    // Continuous painting control
    void startContinuousPainting(Grid* grid, int gridX, int gridY, bool isAlternateMode = false);
    void updatePaintPosition(int gridX, int gridY);
    void stopContinuousPainting();
    [[nodiscard]] bool isPainting() const { return isContinuousPainting; }

    signals:
        void paintApplied();  // Emitted when paint is applied (for triggering UI updates)

private slots:
    void applyPaintAtCurrentPosition();

private:
    void applySingleCell(Cell* cell, bool isAlternateMode) const;

    ToolType currentTool;
    int brushSize;  // Radius of the brush (1 = single cell, 2 = 3x3, 3 = 5x5, etc.)

    // Continuous painting state
    QTimer* paintTimer;
    Grid* currentGrid;
    int currentPaintGridX;
    int currentPaintGridY;
    bool isContinuousPainting;
    bool isAlternateModeActive;  // true when right-clicking
};

#endif  // FLOODSIM_PAINTTOOL_H
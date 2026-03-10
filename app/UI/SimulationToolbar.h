#ifndef FLOODSIM_SIMULATIONTOOLBAR_H
#define FLOODSIM_SIMULATIONTOOLBAR_H

#include <QToolBar>
#include <cstdint>

class Grid;
class FlowModel;
class OpenGLRenderer;
class QLabel;
class Cell;
class QAction;

/**
 * @brief toolbar for simulation controls and cell information display
 *
 * responsible for:
 * - play/pause/step controls
 * - reset functionality
 * - cell information display
 */
class SimulationToolbar : public QToolBar {
    Q_OBJECT

public:
    explicit SimulationToolbar(Grid* grid, FlowModel* flowModel, OpenGLRenderer* renderer, QWidget* parent = nullptr);
    ~SimulationToolbar() override = default;

signals:
    /**
     * @brief display a status message in status bar
     */
    void statusMessageRequested(const QString& message);
    void generateTerrainRequested(uint32_t seed);

private:
    void setupActions();
    void setupCellInfoDisplay();
    void updateCellInfo(int gridX, int gridY, const Cell& cell);

    Grid* grid;
    FlowModel* flowModel;
    OpenGLRenderer* renderer;

    QLabel* cellInfoLabel;
    QAction* viewToggleAction;   // "2D / 3D" toggle button
};

#endif // FLOODSIM_SIMULATIONTOOLBAR_H

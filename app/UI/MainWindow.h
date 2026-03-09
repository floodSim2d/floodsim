#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

class WaterRenderer;
class QLabel;
class Grid;
class FlowModel;
class PaintTool;
class OpenGLRenderer;
class ToolPanel;
class ParameterPanel;
class SimulationToolbar;
class FileMenuHandler;

/**
 * @brief main application window
 *
 * Simulation components:
 * - Grid: simulation grid and heightmap
 * - FlowModel: water flow simulation
 * - PaintTool: terrain and water editing tools
 * - OpenGLRenderer: 3D rendering
 * UI components:
 * - ToolPanel: Tool selection and brush controls
 * - ParameterPanel: Simulation parameters
 * - SimulationToolbar: Simulation controls and cell info
 * - FileMenuHandler: File operations
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onGenerateTerrainRequested(uint32_t seed);

private:
    void setupMenuBar();
    void setupComponents();
    void connectSignals();


    // core simulation components
    std::unique_ptr<Grid> grid;
    std::unique_ptr<WaterRenderer> waterRenderer;
    FlowModel* flowModel;
    PaintTool* paintTool;
    OpenGLRenderer* renderer;

    // ui
    ToolPanel* toolPanel;
    ParameterPanel* parameterPanel;
    SimulationToolbar* simulationToolbar;
    FileMenuHandler* fileMenuHandler;
};

#endif // MAINWINDOW_H

#include "MainWindow.h"

#include <QAction>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QDebug>
#include <QWidget>
#include <QInputDialog>

#include "../Utils/Logger.h"
#include "../Simulation/Grid/Grid.h"
#include "../Simulation/FlowModel/FlowModel.h"
#include "../Simulation/Tools/PaintTool.h"
#include "../Simulation/Tools/TerrainGenerator.h"
#include "../Renderer/OpenGLRenderer.h"
#include "ToolPanel.h"
#include "ParameterPanel.h"
#include "SimulationToolbar.h"
#include "FileMenuHandler.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      grid(nullptr),
      flowModel(nullptr),
      paintTool(nullptr),
      renderer(nullptr),
      toolPanel(nullptr),
      parameterPanel(nullptr),
      simulationToolbar(nullptr),
      fileMenuHandler(nullptr)
{
    grid = std::make_unique<Grid>(200, 200, 1.0F, DEFAULT_WATER_DEPTH);
    paintTool = new PaintTool(this);
    waterRenderer = std::make_unique<WaterRenderer>(grid.get());
    renderer = new OpenGLRenderer(grid.get(), waterRenderer.get(), this);
    flowModel = new FlowModel(grid.get(), this);

    renderer->setPaintTool(paintTool);

    setWindowTitle("FloodSim — Symulator powodzi 3D");

    setupMenuBar();
    setupComponents();
    connectSignals();
}

MainWindow::~MainWindow() {
    LOG("MainWindow destructor - start");

    if (flowModel) {
        LOG("Stopping simulation");
        flowModel->stop();
    }

    // deleting any OpenGL resources
    if (grid && renderer) {
        LOG("Cleaning up OpenGL resources");
        renderer->makeCurrent();
        if (renderer->getWaterRenderer()) {
            renderer->getWaterRenderer()->cleanup();
        }
        grid->cleanup();
        renderer->doneCurrent();
    }

    LOG("Releasing grid");
    grid.reset();

    LOG("MainWindow destructor - end");
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("Plik");

    fileMenuHandler = new FileMenuHandler(grid.get(), renderer, this);

    // File menu
    QAction* newAction = fileMenu->addAction("Nowy");
    QAction* openAction = fileMenu->addAction("Otwórz");
    QAction* saveAction = fileMenu->addAction("Zapisz");
    fileMenu->addSeparator();
    fileMenu->addAction("Wyjście", this, &QMainWindow::close);

    connect(newAction, &QAction::triggered, fileMenuHandler, &FileMenuHandler::handleNew);
    connect(openAction, &QAction::triggered, fileMenuHandler, &FileMenuHandler::handleOpen);
    connect(saveAction, &QAction::triggered, fileMenuHandler, &FileMenuHandler::handleSave);

   // View menu
    QMenu* viewMenu = menuBar()->addMenu("&Widok");
    QAction* resetCameraAction = viewMenu->addAction("Resetuj kamerę");

    connect(resetCameraAction, &QAction::triggered, this, [this]() {
        renderer->resetCamera();
        statusBar()->showMessage("Kamera zresetowana");
    });
}

void MainWindow::setupComponents() {
    toolPanel = new ToolPanel(paintTool, this);
    parameterPanel = new ParameterPanel(grid.get(), flowModel, renderer, this);
    simulationToolbar = new SimulationToolbar(grid.get(), flowModel, renderer, this);

    toolPanel->setMinimumWidth(150);
    parameterPanel->setMinimumWidth(200);

    // Przycisk "Generuj teren" po prawej stronie toolbara symulacji
    simulationToolbar->addSeparator();
    QAction* genAction = simulationToolbar->addAction("🗺 Generuj teren");
    connect(genAction, &QAction::triggered, this, [this]() {
        bool ok = false;
        const int seed = QInputDialog::getInt(
            this,
            "Generator terenu",
            "Seed (liczba całkowita):",
            42, 0, INT_MAX, 1,
            &ok
        );
        if (ok) {
            onGenerateTerrainRequested(static_cast<uint32_t>(seed));
        }
    });

    addToolBar(simulationToolbar);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* mainLayout = new QHBoxLayout(central);
    mainLayout->addWidget(toolPanel);
    mainLayout->addWidget(renderer, 1);
    mainLayout->addWidget(parameterPanel);

    statusBar()->showMessage("Gotowe — narysuj teren lub kliknij 'Generuj teren'");
}

void MainWindow::connectSignals() {
    // FlowModel signals
    connect(flowModel, &FlowModel::simulationStarted, this, [this]() {
        statusBar()->showMessage("Symulacja uruchomiona");
    });

    connect(flowModel, &FlowModel::simulationPaused, this, [this]() {
        statusBar()->showMessage("Symulacja wstrzymana");
    });

    connect(flowModel, &FlowModel::simulationStopped, this, [this]() {
        statusBar()->showMessage("Symulacja zatrzymana");
    });

    connect(flowModel, &FlowModel::stepCompleted, this, [this]() {
        renderer->update();
    });

    // PaintTool signals
    connect(paintTool, &PaintTool::paintApplied, renderer, QOverload<>::of(&QWidget::update));

    // ToolPanel signals
    connect(toolPanel, &ToolPanel::toolSelected, this, [this](const QString& message, bool isCameraMode) {
        renderer->setCameraPanEnabled(isCameraMode);
        statusBar()->showMessage(message);
    });

    // ParameterPanel signals
    connect(parameterPanel, &ParameterPanel::parametersApplied, this, [this](const QString& message) {
        statusBar()->showMessage(message);
    });

    // SimulationToolbar signals
    connect(simulationToolbar, &SimulationToolbar::statusMessageRequested, this, [this](const QString& message) {
        statusBar()->showMessage(message);
    });

    connect(simulationToolbar, &SimulationToolbar::generateTerrainRequested, this, [this](uint32_t seed) {
        flowModel->stop();
        TerrainGenerator gen(seed);
        gen.generateTerrain(*grid);
        gen.printAsciiMap(*grid);
        renderer->makeCurrent();
        grid->updateHeightTexture(); // wypełnij pierwszy PBO
        grid->updateHeightTexture(); // wypełnij drugi PBO (double-buffering)
        renderer->doneCurrent();
        renderer->update();
        statusBar()->showMessage(QString("Teren wygenerowany (seed: %1)").arg(seed));
    });

    // FileMenuHandler signals
    connect(fileMenuHandler, &FileMenuHandler::statusMessageRequested, this, [this](const QString& message) {
        statusBar()->showMessage(message);
    });

    connect(fileMenuHandler, &FileMenuHandler::errorMessageRequested, this,
            [this](const QString& title, const QString& message) {
        QMessageBox::warning(this, title, message);
    });

    // Renderer signals
    connect(renderer, &OpenGLRenderer::cellClicked, this, [this](int gridX, int gridY) {
        statusBar()->showMessage(QString("Kliknięto komórkę: (%1, %2)").arg(gridX).arg(gridY));
    });
}

void MainWindow::onGenerateTerrainRequested(uint32_t seed) {
    flowModel->stop();
    grid->clearHeightmap();
    TerrainGenerator gen(seed);
    gen.generateTerrain(*grid);
    gen.printAsciiMap(*grid);
    renderer->makeCurrent();
    grid->updateHeightTexture(); // wypełnij pierwszy PBO
    grid->updateHeightTexture(); // wypełnij drugi PBO (double-buffering)
    renderer->doneCurrent();
    renderer->update();
    statusBar()->showMessage(QString("Teren wygenerowany (seed: %1)").arg(seed));
}

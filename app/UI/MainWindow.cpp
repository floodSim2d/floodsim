
#include "MainWindow.h"

#include <QAction>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QDebug>
#include <QWidget>

#include "../Simulation/Grid/Grid.h"
#include "../Simulation/FlowModel/FlowModel.h"
#include "../Simulation/Tools/PaintTool.h"
#include "../Renderer/OpenGLRenderer.h"
#include "ToolPanel.h"
#include "ParameterPanel.h"
#include "SimulationToolbar.h"
#include "FileMenuHandler.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      grid(std::make_unique<Grid>(200, 200, 1.0F, DEFAULT_WATER_DEPTH)),
      flowModel(nullptr),
      paintTool(nullptr),
      renderer(nullptr),
      toolPanel(nullptr),
      parameterPanel(nullptr),
      simulationToolbar(nullptr),
      fileMenuHandler(nullptr)
{
    paintTool = new PaintTool(this);
    renderer = new OpenGLRenderer(grid.get(), this);
    flowModel = new FlowModel(grid.get(), this);

    renderer->setPaintTool(paintTool);

    setWindowTitle("FloodSim — Symulator powodzi 3D");

    setupMenuBar();
    setupComponents();
    connectSignals();
}

MainWindow::~MainWindow() {
    qDebug() << "MainWindow destructor - start";

    if (flowModel) {
        qDebug() << "Stopping simulation";
        flowModel->stop();
    }

    // deleting any OpenGL resources
    if (grid && renderer) {
        qDebug() << "Cleaning up OpenGL resources";
        renderer->makeCurrent();
        grid->cleanup();
        renderer->doneCurrent();
    }

    qDebug() << "Releasing grid";
    grid.reset();

    qDebug() << "MainWindow destructor - end";
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

    addToolBar(simulationToolbar);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* mainLayout = new QHBoxLayout(central);
    mainLayout->addWidget(toolPanel);
    mainLayout->addWidget(renderer, 1);  // Stretch factor 1
    mainLayout->addWidget(parameterPanel);

    statusBar()->showMessage("Gotowe");
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


#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "../Simulation/Grid/Grid.h"
#include "../Simulation/FlowModel/FlowModel.h"
#include "../Simulation/Tools/PaintTool.h"
#include "../Renderer/OpenGLRenderer.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      grid(std::make_unique<Grid>(200, 200, 1.0F, DEFAULT_WATER_DEPTH)),
      flowModel(nullptr),
      paintTool(std::make_unique<PaintTool>(this)),
      renderer(nullptr),
      heightLabel(nullptr)
{
    renderer = new OpenGLRenderer(grid.get(), this);

    renderer->setPaintTool(paintTool.get());

    flowModel = std::make_unique<FlowModel>(grid.get(), this);

    // when paint tool applies paint we update the renderer to reflect changes
    connect(paintTool.get(), &PaintTool::paintApplied, renderer, QOverload<>::of(&QWidget::update));

    setWindowTitle("FloodSim — Symulator powodzi 2D");
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    connectFlowModelSignals();

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QHBoxLayout(central);

    QWidget *left  = setupLeftPanel();
    QWidget *right = setupRightPanel();

    // szerokości paneli jak na makiecie
    left->setMinimumWidth(150);
    right->setMinimumWidth(200);

    // dodanie do układu
    mainLayout->addWidget(left);
    mainLayout->addWidget(renderer, 1);
    mainLayout->addWidget(right);
}

MainWindow::~MainWindow() {
    // cleanup opengl resources used in grid first
    renderer->makeCurrent();
    grid->~Grid();
    renderer->doneCurrent();
    delete renderer;
}

void MainWindow::connectFlowModelSignals() {
    connect(flowModel.get(), &FlowModel::simulationStarted, this, [this]() {
        statusBar()->showMessage("Symulacja uruchomiona");
    });

    connect(flowModel.get(), &FlowModel::simulationPaused, this, [this]() {
        statusBar()->showMessage("Symulacja wstrzymana");
    });

    connect(flowModel.get(), &FlowModel::simulationStopped, this, [this]() {
        statusBar()->showMessage("Symulacja zatrzymana");
    });

    connect(flowModel.get(), &FlowModel::stepCompleted, this, [this]() {
        renderer->update();
    });
}

void MainWindow::setupMenuBar() {
    QMenu* viewMenu = menuBar()->addMenu("&Widok");
    QAction* resetCameraAction = viewMenu->addAction("Resetuj kamerę");

    connect(resetCameraAction, &QAction::triggered, this, [this]() {
        renderer->resetCamera();
        statusBar()->showMessage("Kamera zresetowana");
    });

    QMenu *file = menuBar()->addMenu("Plik");

    QAction *newAct   = file->addAction("Nowy");
    QAction *openAct  = file->addAction("Otwórz");
    QAction *saveAct  = file->addAction("Zapisz");
    file->addSeparator();
    file->addAction("Wyjście", this, &QMainWindow::close);

    // otwieranie plików
    connect(openAct, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, "Wczytaj mapę", "", "Mapa (*.map)"
        );

        if (!path.isEmpty()) {
            auto const widthBefore = grid->getWidth();
            auto const heightBefore = grid->getHeight();

            grid->loadHeightmap(path);
            // reset camera if grid size changed
            if (grid->getWidth() != widthBefore || grid->getHeight() != heightBefore) {
                renderer->resetCamera();
            }
        }
    });

    // zapisywanie plików
    connect(saveAct, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this, "Zapisz mapę", "", "Mapa (*.map)"
        );

        if (!path.isEmpty()) {
            grid->saveHeightmap(path);
        }
    });

    // nowa mapa
    connect(newAct, &QAction::triggered, this, [this]() {
        grid->clearHeightmap();
    });
}

void MainWindow::setupToolBar() {
    QToolBar *toolbar = addToolBar("Toolbar");
    const QAction* playAction = toolbar->addAction("▶ Start");
    const QAction* pauseAction = toolbar->addAction("⏸ Stop");
    const QAction* stepAction = toolbar->addAction("⏭ Krok");
    toolbar->addSeparator();
    const QAction * resetAction = toolbar->addAction("Reset");

    // Create height info label in toolbar
    toolbar->addSeparator();
    heightLabel = new QLabel("Wysokość: ---", this);
    heightLabel->setStyleSheet("QLabel { padding: 5px; background-color: rgba(0, 0, 0, 0.1); border-radius: 3px; }");
    heightLabel->setMinimumWidth(200);
    toolbar->addWidget(heightLabel);

    // event listeners
    connect(renderer, &OpenGLRenderer::cellHovered , this, [this](int gridX, int gridY, const Cell& cell) {
        QString label = QString("Wysokość terenu: %1").arg(cell.getTerrainHeight(), 0, 'f', 2);
        if (const auto waterDepth = cell.getWaterDepth(); waterDepth > 0.0F) {
            label += QString(" | Głębokość wody: %1").arg(waterDepth, 0, 'f', 2);
            label += QString(" | Całkowita wysokość: %1").arg(cell.getTotalHeight(), 0, 'f', 2);
        }
        if (const auto velocity = cell.getVelocity(); velocity.length() > 0.01F) {
            label += QString(" | Prędkość wody: (%1, %2)").arg(velocity.x(), 0, 'f', 2).arg(velocity.y(), 0, 'f', 2);
        }

        // Additional flags
        if (cell.getType() == OBSTACLE) {
            label += QString(" | Przeszkoda");
        }
        if (cell.getType() == RIVER) {
            label += QString(" | Rzeka");
        }
        if (cell.getType() == WATER_SOURCE) {
            label += QString(" | Źródło wody (siła: %1)").arg(cell.getSourceStrength(), 0, 'f', 2);
        }
        if (cell.isRainArea()) {
            label += QString(" | Deszcz (intensywność: %1)").arg(cell.getRainIntensity(), 0, 'f', 2);
        }
        if (cell.getWaterDepth() > cell.getRiverCapacity()) {
            label += QString(" | Przelew wody!");
        }
        heightLabel->setText(QString("Pozycja: (%1, %2) | %3").arg(gridX).arg(gridY).arg(label));
    });

    connect(resetAction, &QAction::triggered, this, [this]() {
        flowModel->stop();
        grid->clearHeightmap();
        statusBar()->showMessage("Symulacja zresetowana");
    });

    connect(playAction, &QAction::triggered, this, [this]() {
        flowModel->play();
    });

    connect(pauseAction, &QAction::triggered, this, [this]() {
        flowModel->pause();
    });

    connect(stepAction, &QAction::triggered, this, [this]() {
        flowModel->step();
        statusBar()->showMessage("Krok symulacji wykonany");
    });

}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("Gotowe");


    connect(renderer, &OpenGLRenderer::cellClicked, this, [this](int gridX, int gridY) {
        statusBar()->showMessage(QString("Kliknięto komórkę: (%1, %2)").arg(gridX).arg(gridY));
    });
}

QWidget* MainWindow::setupLeftPanel() {
    auto *panel = new QWidget(this);
    auto *layout = new QVBoxLayout(panel);

    layout->addWidget(new QLabel("Narzędzia:", panel));
    layout->addSpacing(10);

    // tool button configuration
    struct ToolButton {
        QPushButton* button;
        ToolType type;
        QString message;
    };

    auto *btnCameraPan = new QPushButton("Kamera", panel);
    auto *btnTerrain = new QPushButton("Teren", panel);
    auto *btnObstacle = new QPushButton("Przeszkoda", panel);
    auto *btnRiver = new QPushButton("Rzeka", panel);
    auto *btnWaterSource = new QPushButton("Źródło wody", panel);
    auto *btnRain = new QPushButton("Deszcz", panel);
    auto *btnEraser = new QPushButton("Gumka", panel);

    // create array of tool buttons
    std::array<ToolButton, 7> toolButtons = {{
        {btnCameraPan, ToolType::Camera, "Tryb kamery włączony - przeciągnij aby przesunąć widok"},
        {btnTerrain, ToolType::Terrain, "Narzędzie: Teren - kliknij aby podnieść teren"},
        {btnObstacle, ToolType::Obstacle, "Narzędzie: Przeszkoda - kliknij aby umieścić przeszkodę"},
        {btnRiver, ToolType::River, "Narzędzie: Rzeka - kliknij aby utworzyć rzekę"},
        {btnWaterSource, ToolType::WaterSource, "Narzędzie: Źródło wody - stałe źródło utrzymujące poziom wody"},
        {btnRain, ToolType::Rain, "Narzędzie: Deszcz - obszar opadów dodający wodę podczas symulacji"},
        {btnEraser, ToolType::Eraser, "Narzędzie: Gumka - kliknij aby wyczyścić komórkę"}
    }};

    // setup all buttons in a loop
    for (auto& toolBtn : toolButtons) {
        toolBtn.button->setCheckable(true);
        toolBtn.button->setStyleSheet(
            "QPushButton { padding: 8px; font-weight: bold; }"
            "QPushButton:checked { background-color: #4CAF50; color: white; }"
        );
        layout->addWidget(toolBtn.button);
    }

    // brush size slider
    layout->addSpacing(10);
    auto *brushSizeLabel = new QLabel("Rozmiar pędzla:", panel);
    layout->addWidget(brushSizeLabel);

    auto *brushSizeSlider = new QSlider(Qt::Horizontal, panel);
    brushSizeSlider->setMinimum(1);
    brushSizeSlider->setMaximum(10);
    brushSizeSlider->setValue(1);
    brushSizeSlider->setTickPosition(QSlider::TicksBelow);
    brushSizeSlider->setTickInterval(1);
    layout->addWidget(brushSizeSlider);

    auto *brushSizeValueLabel = new QLabel("1", panel);
    brushSizeValueLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(brushSizeValueLabel);

    layout->addStretch();

    connect(brushSizeSlider, &QSlider::valueChanged, this, [this, brushSizeValueLabel](int value) {
        paintTool->setBrushSize(value);
        brushSizeValueLabel->setText(QString::number(value));
    });

    // Connect all tool buttons with a loop
    for (const auto& toolBtn : toolButtons) {
        if (toolBtn.type == ToolType::Camera) {
            // Special handling for camera button
            connect(toolBtn.button, &QPushButton::toggled, this, [this, toolButtons, toolBtn](bool checked) {
                renderer->setCameraPanEnabled(checked);
                if (checked) {
                    for (const auto& btn : toolButtons) {
                        if (btn.type != ToolType::Camera) {
                            btn.button->setChecked(false);
                        }
                    }
                    paintTool->setToolType(ToolType::Camera);
                    statusBar()->showMessage(toolBtn.message);
                }
            });
        } else {
            // Paint tool buttons
            connect(toolBtn.button, &QPushButton::clicked, this, [this, toolButtons, currentTool = toolBtn](bool) {
                // Disable camera mode
                toolButtons[0].button->setChecked(false);
                renderer->setCameraPanEnabled(false);

                paintTool->setToolType(currentTool.type);

                for (const auto& btn : toolButtons) {
                    btn.button->setChecked(btn.button == currentTool.button);
                }

                statusBar()->showMessage(currentTool.message);
            });
        }
    }

    return panel;
}

QWidget* MainWindow::setupRightPanel() {
    auto *panel = new QWidget(this);
    panel->setAutoFillBackground(true);

    auto *panelLayout = new QVBoxLayout(panel);

    auto *grp = new QGroupBox("Parametry", panel);
    auto *groupLayout = new QVBoxLayout(grp);

    auto *spinK = new QDoubleSpinBox(grp);
    spinK->setRange(0, 100);
    spinK->setValue(1);

    groupLayout->addWidget(new QLabel("K:", grp));
    groupLayout->addWidget(spinK);

    // Max depth control with slider
    groupLayout->addWidget(new QLabel("Max głębokość:", grp));

    auto *depthSlider = new QSlider(Qt::Horizontal, grp);
    depthSlider->setMinimum(MIN_WATER_DEPTH);
    depthSlider->setMaximum(MAX_WATER_DEPTH);
    // Use default value (50) since Grid isn't initialized yet in constructor
    // Grid is created in initializeGL() which is called after MainWindow constructor
    depthSlider->setValue(DEFAULT_WATER_DEPTH);
    depthSlider->setTickPosition(QSlider::TicksBelow);
    depthSlider->setTickInterval(10);
    groupLayout->addWidget(depthSlider);

    auto *depthValueLabel = new QLabel(QString::number(depthSlider->value()), grp);
    depthValueLabel->setAlignment(Qt::AlignCenter);
    groupLayout->addWidget(depthValueLabel);

    auto *apply = new QPushButton("Zastosuj", grp);
    groupLayout->addWidget(apply);

    panelLayout->addWidget(grp);
    panelLayout->addStretch();

    // Update label when slider moves (but don't apply yet)
    connect(depthSlider, &QSlider::valueChanged, this, [depthValueLabel](int value) {
        depthValueLabel->setText(QString::number(value));
    });

    // Apply changes only when button is clicked
    connect(apply, &QPushButton::clicked, this, [this, spinK, depthSlider]() {
        // Apply K value
        const auto kValue = static_cast<float>(spinK->value());
        flowModel->setFlowCoefficient(kValue);

        // Apply maxDepth value
        const auto newDepth = static_cast<float>(depthSlider->value());
        grid->setMaxDepth(newDepth);
        renderer->updateProjectionMatrix();

        statusBar()->showMessage(QString("Parametry zastosowane: K=%1, Max głębokość=%2")
            .arg(kValue, 0, 'f', 2).arg(newDepth, 0, 'f', 1));
    });

    return panel;
}

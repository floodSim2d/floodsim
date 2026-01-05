
#include "MainWindow.h"

#include <QAction>
#include <QButtonGroup>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QToolBar>
#include <QHBoxLayout>
#include <QDebug>
#include <QWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>

#include "../Utils/Logger.h"
#include "../Simulation/Grid/Grid.h"
#include "../Simulation/FlowModel/FlowModel.h"
#include "../Simulation/Tools/PaintTool.h"
#include "../Renderer/OpenGLRenderer.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      grid(std::make_unique<Grid>(200, 200, 1.0F, DEFAULT_WATER_DEPTH)),
      flowModel(nullptr),
      paintTool(nullptr),
      renderer(nullptr),
      heightLabel(nullptr)
{
    // Tworzymy obiekty Qt przez new z parent=this — Qt zajmie się ich usunięciem
    paintTool = new PaintTool(this);
    renderer = new OpenGLRenderer(grid.get(), this);
    flowModel = new FlowModel(grid.get(), this);

    renderer->setPaintTool(paintTool);

    // when paint tool applies paint we update the renderer to reflect changes
    connect(paintTool, &PaintTool::paintApplied, renderer, QOverload<>::of(&QWidget::update));

    setWindowTitle("FloodSim — Symulator powodzi 3D");
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
    mainLayout->addWidget(renderer, 1);  // renderer jest Widgetem zarządzanym przez Qt
    mainLayout->addWidget(right);
}

MainWindow::~MainWindow() {
    LOG("MainWindow destructor - start");

    if (flowModel) {
        LOG("Stopping simulation");
        flowModel->stop();
    }

    if (grid && renderer) {
        LOG("Cleaning up OpenGL resources");
        renderer->makeCurrent();
        grid->cleanup();
        renderer->doneCurrent();
    }

    LOG("Releasing grid");
    grid.reset();

    LOG("MainWindow destructor - end");
}

void MainWindow::connectFlowModelSignals() {
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

            if (! grid->loadHeightmap(path)) {
                QMessageBox::warning(this, "Błąd wczytywania",
                    "Nie udało się wczytać pliku mapy.\n\n"
                    "Plik może być uszkodzony lub mieć nieprawidłowy format.");
                statusBar()->showMessage("Błąd wczytywania pliku");
                return;
            }

            // reset camera if grid size changed
            if (grid->getWidth() != widthBefore || grid->getHeight() != heightBefore) {
                renderer->resetCamera();
            }
            statusBar()->showMessage(QString("Wczytano mapę: %1").arg(path));
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
    QAction* playAction = toolbar->addAction("▶ Start");
    QAction* pauseAction = toolbar->addAction("⏸ Stop");
    QAction* stepAction = toolbar->addAction("⏭ Krok");
    toolbar->addSeparator();
    QAction * resetAction = toolbar->addAction("Reset");

    // Create height info label in toolbar
    toolbar->addSeparator();
    heightLabel = new QLabel("Wysokość: ---", this);
    heightLabel->setStyleSheet("QLabel { padding: 5px; background-color: rgba(0, 0, 0, 0.1); border-radius: 3px; }");
    heightLabel->setMinimumWidth(200);
    toolbar->addWidget(heightLabel);

    // event listeners
    connect(renderer, &OpenGLRenderer::cellHovered, this, [this](int gridX, int gridY, const Cell& cell) {
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
            label += QString(" | Źródło wody (siła:  %1)").arg(cell.getSourceStrength(), 0, 'f', 2);
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
        grid->updateHeightTexture();
        renderer->update();
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
        statusBar()->showMessage(QString("Kliknięto komórkę:  (%1, %2)").arg(gridX).arg(gridY));
    });
}

QWidget* MainWindow::setupLeftPanel() {
    auto *panel = new QWidget(this);
    auto *layout = new QVBoxLayout(panel);

    layout->addWidget(new QLabel("Narzędzia:", panel));
    layout->addSpacing(10);

    auto* buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);

    struct ToolButtonInfo {
        QString name;
        ToolType type;
        QString message;
    };

    std::vector<ToolButtonInfo> tools = {
        {"Kamera", ToolType::Camera, "Tryb kamery włączony - nawiguj sceną 3D"},
        {"Teren", ToolType::Terrain, "Narzędzie:  Teren - kliknij aby podnieść teren"},
        {"Przeszkoda", ToolType::Obstacle, "Narzędzie: Przeszkoda - kliknij aby umieścić przeszkodę"},
        {"Rzeka", ToolType::River, "Narzędzie: Rzeka - kliknij aby utworzyć rzekę"},
        {"Źródło wody", ToolType::WaterSource, "Narzędzie: Źródło wody - stałe źródło utrzymujące poziom wody"},
        {"Gumka", ToolType::Eraser, "Narzędzie: Gumka - kliknij aby wyczyścić komórkę"}
    };

    for (const auto& toolInfo : tools) {
        auto* button = new QPushButton(toolInfo.name, panel);
        button->setCheckable(true);
        button->setStyleSheet(
            "QPushButton { padding: 8px; font-weight: bold; }"
            "QPushButton:checked { background-color: #4CAF50; color: white; }"
        );
        layout->addWidget(button);
        buttonGroup->addButton(button);

        connect(button, &QPushButton::clicked, this, [this, toolInfo]() {
            paintTool->setToolType(toolInfo.type);
            renderer->setCameraPanEnabled(toolInfo.type == ToolType::Camera);
            statusBar()->showMessage(toolInfo.message);
        });
    }

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

    buttonGroup->buttons().at(1)->setChecked(true);
    paintTool->setToolType(tools[1].type);
    renderer->setCameraPanEnabled(false);

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

    groupLayout->addWidget(new QLabel("Max głębokość:", grp));

    auto *depthSlider = new QSlider(Qt::Horizontal, grp);
    depthSlider->setMinimum(MIN_WATER_DEPTH);
    depthSlider->setMaximum(MAX_WATER_DEPTH);
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

    // --- Pogoda (Deszcz) ---
    auto *weatherGrp = new QGroupBox("Pogoda", panel);
    auto *weatherLayout = new QVBoxLayout(weatherGrp);

    auto *rainInfo = new QLabel("Symuluje opady na całej powierzchni mapy. Suwak określa przyrost wody w metrach na sekundę (max 0.5 m/s).", weatherGrp);
    rainInfo->setWordWrap(true);
    rainInfo->setStyleSheet("QLabel { color: #666; font-size: 11px; margin-bottom: 5px; }");
    weatherLayout->addWidget(rainInfo);

    auto *rainCheck = new QCheckBox("Globalny deszcz", weatherGrp);
    weatherLayout->addWidget(rainCheck);

    weatherLayout->addWidget(new QLabel("Intensywność opadów:", weatherGrp));
    auto *rainSlider = new QSlider(Qt::Horizontal, weatherGrp);
    rainSlider->setMinimum(0);
    rainSlider->setMaximum(100);
    rainSlider->setValue(0);
    weatherLayout->addWidget(rainSlider);

    auto *rainValueLabel = new QLabel("0.0", weatherGrp);
    rainValueLabel->setAlignment(Qt::AlignCenter);
    weatherLayout->addWidget(rainValueLabel);

    panelLayout->addWidget(weatherGrp);
    panelLayout->addStretch();

    connect(depthSlider, &QSlider::valueChanged, this, [depthValueLabel](int value) {
        depthValueLabel->setText(QString::number(value));
    });

    connect(apply, &QPushButton::clicked, this, [this, spinK, depthSlider]() {
        const auto kValue = static_cast<float>(spinK->value());
        flowModel->setFlowCoefficient(kValue);

        const auto newDepth = static_cast<float>(depthSlider->value());
        grid->setMaxDepth(newDepth);
        renderer->updateProjectionMatrix();

        statusBar()->showMessage(QString("Parametry zastosowane: K=%1, Max głębokość=%2")
            .arg(kValue, 0, 'f', 2).arg(newDepth, 0, 'f', 1));
    });

    // Rain connections
    connect(rainCheck, &QCheckBox::toggled, this, [this](bool checked) {
        flowModel->setGlobalRainEnabled(checked);
        if (checked) {
            statusBar()->showMessage("Włączono globalne opady deszczu");
        } else {
            statusBar()->showMessage("Wyłączono opady deszczu");
        }
    });

    connect(rainSlider, &QSlider::valueChanged, this, [this, rainValueLabel](int value) {
        // Map 0-100 slider to 0.0 - 0.5 intensity
        float intensity = static_cast<float>(value) / 200.0f; 
        flowModel->setGlobalRainIntensity(intensity);
        rainValueLabel->setText(QString::number(intensity, 'f', 3));
    });

    return panel;
}

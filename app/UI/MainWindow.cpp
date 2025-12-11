#include "MainWindow.h"

#include <QAction>
#include <QButtonGroup>
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
#include "../Renderer/OpenGLRenderer.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    renderer = new OpenGLRenderer(this);

    setWindowTitle("FloodSim — Symulator powodzi 2D");
    setupMenuBar();
    setupToolBar();
    setupStatusBar();

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QHBoxLayout(central);

    QWidget *left  = setupLeftPanel();
    QWidget *right = setupRightPanel();

    left->setMinimumWidth(150);
    right->setMinimumWidth(200);

    mainLayout->addWidget(left);
    mainLayout->addWidget(renderer, 1);
    mainLayout->addWidget(right);
}

void MainWindow::setupMenuBar() {
    QMenu* viewMenu = menuBar()->addMenu("&Widok");
    QAction* resetCameraAction = viewMenu->addAction("Resetuj kamerę");
    connect(resetCameraAction, &QAction::triggered, renderer, &OpenGLRenderer::resetCamera);

    QMenu *file = menuBar()->addMenu("Plik");
    QAction *newAct   = file->addAction("Nowy");
    QAction *openAct  = file->addAction("Otwórz");
    QAction *saveAct  = file->addAction("Zapisz");
    file->addSeparator();
    file->addAction("Wyjście", this, &QMainWindow::close);

    connect(openAct, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Wczytaj mapę", "", "Mapa (*.map)");
        if (!path.isEmpty()) {
            renderer->getGrid()->loadHeightmap(path);
            renderer->resetCamera();
        }
    });

    connect(saveAct, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, "Zapisz mapę", "", "Mapa (*.map)");
        if (!path.isEmpty()) {
            renderer->getGrid()->saveHeightmap(path);
        }
    });

    connect(newAct, &QAction::triggered, this, [this]() {
        renderer->getGrid()->clearHeightmap();
    });
}

void MainWindow::setupToolBar() {
    QToolBar *toolbar = addToolBar("Toolbar");
    toolbar->addAction("▶ Start");
    toolbar->addAction("⏸ Stop");
    toolbar->addAction("⏭ Krok");
    toolbar->addSeparator();
    toolbar->addAction("Reset");

    heightLabel = new QLabel("Wysokość: ---", this);
    heightLabel->setStyleSheet("QLabel { padding: 5px; background-color: rgba(0, 0, 0, 0.1); border-radius: 3px; }");
    heightLabel->setMinimumWidth(200);
    toolbar->addWidget(heightLabel);

    connect(renderer, &OpenGLRenderer::cellHovered , this, [this](int gridX, int gridY, const Cell& cell) {
        QString label = QString("Wysokość terenu: %1").arg(cell.getTerrainHeight(), 0, 'f', 2);
        heightLabel->setText(QString("Pozycja: (%1, %2) | %3").arg(gridX).arg(gridY).arg(label));
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

    auto* buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);

    struct ToolButtonInfo {
        QString name;
        ToolType type;
        QString message;
    };

    std::vector<ToolButtonInfo> tools = {
        {"Kamera", ToolType::Camera, "Tryb kamery: Nawiguj sceną"},
        {"Teren", ToolType::Terrain, "Narzędzie: Teren"},
        {"Przeszkoda", ToolType::Obstacle, "Narzędzie: Przeszkoda"},
        {"Rzeka", ToolType::River, "Narzędzie: Rzeka"},
        {"Źródło wody", ToolType::WaterSource, "Narzędzie: Źródło wody"},
        {"Gumka", ToolType::Eraser, "Narzędzie: Gumka"}
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
            renderer->getPaintTool()->setToolType(toolInfo.type);
            renderer->setCameraPanEnabled(toolInfo.type == ToolType::Camera);
            statusBar()->showMessage(toolInfo.message);
        });
    }

    // Domyślnie zaznaczony przycisk "Kamera"
    buttonGroup->buttons().at(0)->setChecked(true);
    renderer->getPaintTool()->setToolType(tools[0].type);
    renderer->setCameraPanEnabled(true);

    layout->addSpacing(10);
    auto *brushSizeLabel = new QLabel("Rozmiar pędzla:", panel);
    layout->addWidget(brushSizeLabel);

    auto *brushSizeSlider = new QSlider(Qt::Horizontal, panel);
    brushSizeSlider->setRange(1, 10);
    brushSizeSlider->setValue(1);
    layout->addWidget(brushSizeSlider);

    connect(brushSizeSlider, &QSlider::valueChanged, renderer->getPaintTool(), &PaintTool::setBrushSize);

    layout->addStretch();
    return panel;
}

QWidget* MainWindow::setupRightPanel() {
    auto *panel = new QWidget(this);
    auto *panelLayout = new QVBoxLayout(panel);
    auto *grp = new QGroupBox("Parametry", panel);
    auto *groupLayout = new QVBoxLayout(grp);

    groupLayout->addWidget(new QLabel("Max głębokość:", grp));
    auto *depthSlider = new QSlider(Qt::Horizontal, grp);
    depthSlider->setRange(MIN_WATER_DEPTH, MAX_WATER_DEPTH);
    depthSlider->setValue(DEFAULT_WATER_DEPTH);
    groupLayout->addWidget(depthSlider);

    connect(depthSlider, &QSlider::valueChanged, this, [this](int value) {
        auto* grid = renderer->getGrid();
        if (grid) {
            grid->setMaxDepth(static_cast<float>(value));
            renderer->resetCamera();
        }
    });

    panelLayout->addWidget(grp);
    panelLayout->addStretch();
    return panel;
}

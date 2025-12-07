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
#include "../Renderer/OpenGLRenderer.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), renderer(nullptr), heightLabel(nullptr)
{
    renderer = new OpenGLRenderer();

    setWindowTitle("FloodSim — Symulator powodzi 2D");
    setupMenuBar();
    setupToolBar();
    setupStatusBar();

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

void MainWindow::setupMenuBar() {
    // widok
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
            auto const widthBefore = renderer->getGrid()->getWidth();
            auto const heightBefore = renderer->getGrid()->getHeight();

            renderer->getGrid()->loadHeightmap(path);
            // reset camera if grid size changed
            if (renderer->getGrid()->getWidth() != widthBefore || renderer->getGrid()->getHeight() != heightBefore) {
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
            renderer->getGrid()->saveHeightmap(path);
        }
    });

    // nowa mapa
    connect(newAct, &QAction::triggered, this, [this]() {
        renderer->getGrid()->clearHeightmap();
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
        if (cell.isObstacle()) {
            label += QString(" | Przeszkoda");
        }
        if (cell.isRiver()) {
            label += QString(" | Rzeka");
        }
        if (cell.isWaterSource()) {
            label += QString(" | Źródło wody");
        }
        if (cell.getWaterDepth() > cell.getRiverCapacity()) {
            label += QString(" | Przelew wody!");
        }
        heightLabel->setText(QString("Pozycja: (%1, %2) | %3").arg(gridX).arg(gridY).arg(label));
    });

    connect(resetAction, &QAction::triggered, this, [this]() {
       statusBar()->showMessage("Symulacja zresetowana");
   });

    connect(playAction, &QAction::triggered, this, [this]() { statusBar()->showMessage("Symulacja uruchomiona"); });

    connect(pauseAction, &QAction::triggered, this, [this]() { statusBar()->showMessage("Symulacja zatrzymana"); });

    connect(stepAction, &QAction::triggered, this, [this]() { statusBar()->showMessage("Krok symulacji"); });

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

    // Tool button configuration
    struct ToolButton {
        QPushButton* button;
        ToolType type;
        QString message;
    };

    // Create buttons
    auto *btnCameraPan = new QPushButton("Kamera", panel);
    auto *btnTerrain = new QPushButton("Teren", panel);
    auto *btnObstacle = new QPushButton("Przeszkoda", panel);
    auto *btnRiver = new QPushButton("Rzeka", panel);
    auto *btnRain = new QPushButton("Źródło wody", panel);
    auto *btnEraser = new QPushButton("Gumka", panel);

    // Configure tool buttons with their types and messages
    std::vector<ToolButton> toolButtons = {
        {btnCameraPan, ToolType::Camera, "Tryb kamery włączony - przeciągnij aby przesunąć widok"},
        {btnTerrain, ToolType::Terrain, "Narzędzie: Teren - kliknij aby podnieść teren"},
        {btnObstacle, ToolType::Obstacle, "Narzędzie: Przeszkoda - kliknij aby umieścić przeszkodę"},
        {btnRiver, ToolType::River, "Narzędzie: Rzeka - kliknij aby utworzyć rzekę"},
        {btnRain, ToolType::WaterSource, "Narzędzie: Źródło wody - kliknij aby dodać źródło wody"},
        {btnEraser, ToolType::Eraser, "Narzędzie: Gumka - kliknij aby wyczyścić komórkę"}
    };

    // Setup all buttons
    for (auto& toolBtn : toolButtons) {
        toolBtn.button->setCheckable(true);
        toolBtn.button->setStyleSheet(
            "QPushButton { padding: 8px; font-weight: bold; }"
            "QPushButton:checked { background-color: #4CAF50; color: white; }"
        );
        layout->addWidget(toolBtn.button);
    }

    // Brush size slider
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
        renderer->getPaintTool()->setBrushSize(value);
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
                    renderer->getPaintTool()->setToolType(ToolType::Camera);
                    statusBar()->showMessage(toolBtn.message);
                }
            });
        } else {
            // Paint tool buttons
            connect(toolBtn.button, &QPushButton::clicked, this, [this, toolButtons, currentTool = toolBtn](bool) {
                // Disable camera mode
                toolButtons[0].button->setChecked(false);
                renderer->setCameraPanEnabled(false);

                renderer->getPaintTool()->setToolType(currentTool.type);

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

    auto *spinDepth = new QSpinBox(grp);
    spinDepth->setRange(1, 100);
    spinDepth->setValue(10);

    groupLayout->addWidget(new QLabel("K:", grp));
    groupLayout->addWidget(spinK);
    groupLayout->addWidget(new QLabel("Max głębokość:", grp));
    groupLayout->addWidget(spinDepth);

    auto *apply = new QPushButton("Zastosuj", grp);
    groupLayout->addWidget(apply);

    panelLayout->addWidget(grp);
    panelLayout->addStretch();

    return panel;
}

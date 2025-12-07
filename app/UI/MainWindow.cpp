#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "../Simulation/Grid/Grid.h"

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
    QAction* playAction = toolbar->addAction("▶ Start");
    QAction* pauseAction = toolbar->addAction("⏸ Stop");
    QAction* stepAction = toolbar->addAction("⏭ Krok");
    toolbar->addSeparator();
    QAction* resetAction = toolbar->addAction("Reset");

    // Create height info label in toolbar
    toolbar->addSeparator();
    heightLabel = new QLabel("Wysokość: ---", this);
    heightLabel->setStyleSheet("QLabel { padding: 5px; background-color: rgba(0, 0, 0, 0.1); border-radius: 3px; }");
    heightLabel->setMinimumWidth(200);
    toolbar->addWidget(heightLabel);

    // event listeners
    connect(renderer, &OpenGLRenderer::cellHovered , this, [this](int gridX, int gridY, const Cell& cell) {
        QString label = QString("Wysokość terenu: %1").arg(cell.getTerrainHeight(), 0, 'f', 2);
        if (cell.getWaterDepth() > 0.0f) {
            label += QString(" | Głębokość wody: %1").arg(cell.getWaterDepth(), 0, 'f', 2);
            label += QString(" | Całkowita wysokość: %1").arg(cell.getTotalHeight(), 0, 'f', 2);
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
    auto *v = new QVBoxLayout(panel);

    auto *btnTerrain   = new QPushButton("Teren");
    auto *btnObstacle  = new QPushButton("Przeszkoda");
    auto *btnRiver     = new QPushButton("Rzeka");
    auto *btnRain      = new QPushButton("Źródło wody");
    auto *btnEraser    = new QPushButton("Gumka");

    v->addWidget(btnTerrain);
    v->addWidget(btnObstacle);
    v->addWidget(btnRiver);
    v->addWidget(btnRain);
    v->addWidget(btnEraser);
    v->addStretch();

    // TODO: replace with OpenGLRenderer class functions
    // connect(btnTerrain,  &QPushButton::clicked, [this](){ mapView->setTool(MapView::Tool::Terrain); });
    // connect(btnObstacle, &QPushButton::clicked, [this](){ mapView->setTool(MapView::Tool::Obstacle); });
    // connect(btnRiver,    &QPushButton::clicked, [this](){ mapView->setTool(MapView::Tool::River); });
    // connect(btnRain,     &QPushButton::clicked, [this](){ mapView->setTool(MapView::Tool::WaterSource); });
    // connect(btnEraser,   &QPushButton::clicked, [this](){ mapView->setTool(MapView::Tool::Eraser); });

    return panel;
}

QWidget* MainWindow::setupRightPanel() {
    auto *panel = new QWidget(this);

    panel->setAutoFillBackground(true);

    auto *v = new QVBoxLayout(panel);

    auto *grp = new QGroupBox("Parametry", panel);
    auto *g = new QVBoxLayout(grp);

    auto *const spinK = new QDoubleSpinBox();
    spinK->setRange(0, 100);
    spinK->setValue(1);

    const auto spinDepth = new QSpinBox();
    spinDepth->setRange(1, 100);
    spinDepth->setValue(10);

    g->addWidget(new QLabel("K:"));
    g->addWidget(spinK);
    g->addWidget(new QLabel("Max głębokość:"));
    g->addWidget(spinDepth);

    auto *const apply = new QPushButton("Zastosuj");
    g->addWidget(apply);

    v->addWidget(grp);
    v->addStretch();

    return panel;
}

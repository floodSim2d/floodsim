#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "../Renderer/OpenGLRenderer.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), renderer(nullptr), heightLabel(nullptr) {
    setWindowTitle("FloodSim — Symulator powodzi 2D");
    resize(1200, 800);

    // Menu Plik
    QMenu* fileMenu = menuBar()->addMenu("&Plik");
    fileMenu->addAction("Nowa mapa");
    fileMenu->addAction("Otwórz...");
    fileMenu->addAction("Zapisz");
    fileMenu->addSeparator();
    fileMenu->addAction("Wyjście", this, &QMainWindow::close);

    // Menu Widok
    QMenu* viewMenu = menuBar()->addMenu("&Widok");
    QAction* resetCameraAction = viewMenu->addAction("Resetuj kamerę");

    // Pasek narzędzi symulacji
    QToolBar* toolbar = addToolBar("Symulacja");
    QAction* playAction = toolbar->addAction("▶ Play");
    QAction* pauseAction = toolbar->addAction("⏸ Pause");
    QAction* stepAction = toolbar->addAction("⏭ Step");
    toolbar->addSeparator();
    QAction* resetAction = toolbar->addAction("🔄 Reset");

    // Create height info label in toolbar
    toolbar->addSeparator();
    heightLabel = new QLabel("Wysokość: ---", this);
    heightLabel->setStyleSheet("QLabel { padding: 5px; background-color: rgba(0, 0, 0, 0.1); border-radius: 3px; }");
    heightLabel->setMinimumWidth(200);
    toolbar->addWidget(heightLabel);

    // Statusbar (na dole okna)
    statusBar()->showMessage("Gotowy");

    renderer = new OpenGLRenderer(this);
    setCentralWidget(renderer);

    // Connect actions
    connect(playAction, &QAction::triggered, this, [this]() { statusBar()->showMessage("Symulacja uruchomiona"); });

    connect(pauseAction, &QAction::triggered, this, [this]() { statusBar()->showMessage("Symulacja zatrzymana"); });

    connect(stepAction, &QAction::triggered, this, [this]() { statusBar()->showMessage("Krok symulacji"); });

    connect(resetAction, &QAction::triggered, this, [this]() {
        renderer->resetCamera();
        statusBar()->showMessage("Kamera zresetowana");
    });

    connect(resetCameraAction, &QAction::triggered, this, [this]() {
        renderer->resetCamera();
        statusBar()->showMessage("Kamera zresetowana");
    });

    // Display height value on hover
    connect(renderer, &OpenGLRenderer::heightValueHovered, this, [this](int gridX, int gridY, float height) {
        heightLabel->setText(QString("Pozycja: (%1, %2) | Wysokość: %3").arg(gridX).arg(gridY).arg(height, 0, 'f', 2));
    });

    // Display cell coordinates on click
    connect(renderer, &OpenGLRenderer::cellClicked, this, [this](int gridX, int gridY) {
        statusBar()->showMessage(QString("Kliknięto komórkę: (%1, %2)").arg(gridX).arg(gridY));
    });
}

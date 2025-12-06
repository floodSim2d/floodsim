#include "MainWindow.h"
#include "MapView.h"
#include "../Renderer/GLRenderer.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include "../Simulation/Grid.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_grid = std::make_shared<Grid>(50, 50);
    setupMenuBar();
    setupToolBar();
    setupStatusBar();

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);

    QWidget *left  = setupLeftPanel();
    // Tymczasowo zamieniamy widok 2D na renderer OpenGL
    auto* glView = new GLRenderer(m_grid, this);
    // Ustawiamy mapView na null, aby uniknąć błędów przy klikaniu przycisków
    mapView = nullptr;
    QWidget *right = setupRightPanel();

    // szerokości paneli jak na makiecie
    left->setMinimumWidth(150);
    right->setMinimumWidth(200);

    // dodanie do układu
    mainLayout->addWidget(left);
    mainLayout->addWidget(glView, 1); // mapa zajmuje resztę
    mainLayout->addWidget(right);
}

void MainWindow::setupMenuBar() {
    QMenu *file = menuBar()->addMenu("Plik");

    QAction *newAct   = file->addAction("Nowy");
    QAction *openAct  = file->addAction("Otwórz");
    QAction *saveAct  = file->addAction("Zapisz");

    // 🔵 NOWY — otwieranie plików
    connect(openAct, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, "Wczytaj mapę", "", "Mapa (*.map)"
        );

        if (!path.isEmpty()) {
            m_grid->loadFromFile(path);
            // Nie trzeba odświeżać, bo GLRenderer robi to w każdej klatce
            // if (mapView) mapView->update();
        }
    });

    connect(saveAct, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this, "Zapisz mapę", "", "Mapa (*.map)"
        );

        if (!path.isEmpty())
            m_grid->saveToFile(path);
    });

    // (opcjonalnie) NOWY — nowa mapa
    connect(newAct, &QAction::triggered, this, [this]() {
        m_grid->clear();
        // if (mapView) mapView->update();
    });
}

void MainWindow::setupToolBar() {
    QToolBar *tb = addToolBar("Toolbar");
    tb->addAction("Start");
    tb->addAction("Stop");
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("Gotowe");
}

QWidget* MainWindow::setupLeftPanel() {
    QWidget *panel = new QWidget(this);
    QVBoxLayout *v = new QVBoxLayout(panel);

    QPushButton *btnTerrain   = new QPushButton("Teren");
    QPushButton *btnObstacle  = new QPushButton("Przeszkoda");
    QPushButton *btnRiver     = new QPushButton("Rzeka");
    QPushButton *btnRain      = new QPushButton("Źródło wody");
    QPushButton *btnEraser    = new QPushButton("Gumka");

    v->addWidget(btnTerrain);
    v->addWidget(btnObstacle);
    v->addWidget(btnRiver);
    v->addWidget(btnRain);
    v->addWidget(btnEraser);

    v->addSpacing(20); // Trochę odstępu
    v->addWidget(new QLabel("Rozmiar pędzla:"));
    QSpinBox *brushSizeSpinBox = new QSpinBox();
    brushSizeSpinBox->setRange(1, 21); // Pędzle od 1x1 do 21x21
    brushSizeSpinBox->setSingleStep(2); // Krok co 2, aby mieć nieparzyste rozmiary (1, 3, 5...)
    brushSizeSpinBox->setValue(1);
    v->addWidget(brushSizeSpinBox);

    v->addStretch();

    // Tymczasowo wyłączamy połączenia, ponieważ mapView jest nieaktywny
    // connect(btnTerrain,  &QPushButton::clicked, [this](){ if(mapView) mapView->setTool(MapView::Tool::Terrain); });
    // connect(btnObstacle, &QPushButton::clicked, [this](){ if(mapView) mapView->setTool(MapView::Tool::Obstacle); });
    // connect(btnRiver,    &QPushButton::clicked, [this](){ if(mapView) mapView->setTool(MapView::Tool::River); });
    // connect(btnRain,     &QPushButton::clicked, [this](){ if(mapView) mapView->setTool(MapView::Tool::WaterSource); });
    // connect(btnEraser,   &QPushButton::clicked, [this](){ if(mapView) mapView->setTool(MapView::Tool::Eraser); });

    // --- NOWY KOD: Podłączenie sygnału zmiany rozmiaru pędzla ---
    // connect(brushSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value){ if(mapView) mapView->setBrushSize(value); });

    return panel;
}

QWidget* MainWindow::setupRightPanel() {
    QWidget *panel = new QWidget(this);

    panel->setAutoFillBackground(true);

    QVBoxLayout *v = new QVBoxLayout(panel);

    QGroupBox *grp = new QGroupBox("Parametry", panel);
    QVBoxLayout *g = new QVBoxLayout(grp);

    QDoubleSpinBox *spinK = new QDoubleSpinBox();
    spinK->setRange(0, 100);
    spinK->setValue(1);

    QSpinBox *spinDepth = new QSpinBox();
    spinDepth->setRange(1, 100);
    spinDepth->setValue(10);

    g->addWidget(new QLabel("K:"));
    g->addWidget(spinK);
    g->addWidget(new QLabel("Max głębokość:"));
    g->addWidget(spinDepth);

    QPushButton *apply = new QPushButton("Zastosuj");
    g->addWidget(apply);

    v->addWidget(grp);
    v->addStretch();

    return panel;
}

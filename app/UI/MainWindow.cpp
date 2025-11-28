#include "MainWindow.h"
#include "MapView.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupMenuBar();
    setupToolBar();
    setupStatusBar();

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);

    QWidget *left  = setupLeftPanel();
    mapView        = new MapView(this);
    QWidget *right = setupRightPanel();

    // szerokości paneli jak na makiecie
    left->setMinimumWidth(150);
    right->setMinimumWidth(200);

    // dodanie do układu
    mainLayout->addWidget(left);
    mainLayout->addWidget(mapView, 1); // mapa zajmuje resztę
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

        if (!path.isEmpty())
            mapView->loadFromFile(path);
    });

    // 🔵 NOWY — zapisywanie plików
    connect(saveAct, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this, "Zapisz mapę", "", "Mapa (*.map)"
        );

        if (!path.isEmpty())
            mapView->saveToFile(path);
    });

    // (opcjonalnie) NOWY — nowa mapa
    connect(newAct, &QAction::triggered, this, [this]() {
        mapView->clearMap();
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
    v->addStretch();

    connect(btnTerrain,  &QPushButton::clicked, [this](){ mapView->setTool(MapView::Tool::Terrain); });
    connect(btnObstacle, &QPushButton::clicked, [this](){ mapView->setTool(MapView::Tool::Obstacle); });
    connect(btnRiver,    &QPushButton::clicked, [this](){ mapView->setTool(MapView::Tool::River); });
    connect(btnRain,     &QPushButton::clicked, [this](){ mapView->setTool(MapView::Tool::WaterSource); });
    connect(btnEraser,   &QPushButton::clicked, [this](){ mapView->setTool(MapView::Tool::Eraser); });

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



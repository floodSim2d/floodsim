#include "MainWindow.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("FloodSim — Edytor mapy (Milestone 1)");
    resize(1000, 700);

    // Menu Plik
    QMenu *fileMenu = menuBar()->addMenu("&Plik");
    fileMenu->addAction("Nowa mapa");
    fileMenu->addAction("Otwórz...");
    fileMenu->addAction("Zapisz");

    // Pasek narzędzi
    QToolBar *toolbar = addToolBar("Symulacja");
    QAction *playAction = toolbar->addAction("Play");
    QAction *pauseAction = toolbar->addAction("Pause");
    QAction *stepAction = toolbar->addAction("Step");

    // Statusbar (na dole okna)
    statusBar()->showMessage("Gotowy");

    // Prosty środkowy obszar — placeholder na mapę
    QWidget *placeholder = new QWidget(this);
    placeholder->setStyleSheet("background-color: #d0d0d0; border: 1px solid #808080;");
    setCentralWidget(placeholder);

    // Kiedy klikniesz przycisk — zmienia się status
    connect(playAction, &QAction::triggered, this, [this]() {
        statusBar()->showMessage("Play!");
    });
    connect(pauseAction, &QAction::triggered, this, [this]() {
        statusBar()->showMessage("Pause!");
    });
    connect(stepAction, &QAction::triggered, this, [this]() {
        statusBar()->showMessage("Step!");
    });
}

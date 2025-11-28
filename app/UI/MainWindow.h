#pragma once
#include <QMainWindow>

class MapView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupMenuBar();
    void setupToolBar();
    QWidget* setupLeftPanel();
    QWidget* setupRightPanel();
    void setupStatusBar();

    MapView *mapView;
};

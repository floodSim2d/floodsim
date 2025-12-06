#pragma once
#include <QMainWindow>
#include <memory>


class MapView;
class Grid;
class GLRenderer;
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
    GLRenderer *glRenderer;
    std::shared_ptr<Grid> m_grid;
};

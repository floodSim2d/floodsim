#pragma once
#include <QMainWindow>
#include <QLabel>
#include "../Renderer/OpenGLRenderer.h"

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

    OpenGLRenderer* renderer;
    QLabel* heightLabel;
};

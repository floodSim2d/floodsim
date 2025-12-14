#pragma once
#include <QMainWindow>
#include <QLabel>
#include <memory>

class OpenGLRenderer;
class Grid;
class FlowModel;
class PaintTool;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupMenuBar();
    void setupToolBar();
    QWidget* setupLeftPanel();
    QWidget* setupRightPanel();
    void setupStatusBar();
    void connectFlowModelSignals();

    // Core components
    std::unique_ptr<Grid> grid;
    std::unique_ptr<FlowModel> flowModel;
    std::unique_ptr<PaintTool> paintTool;

    // UI components
    OpenGLRenderer* renderer;
    QLabel* heightLabel;
};

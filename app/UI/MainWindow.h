#include <QMainWindow>
#include <memory>

class QLabel;
class Grid;
class FlowModel;
class PaintTool;
class OpenGLRenderer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void connectFlowModelSignals();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    QWidget* setupLeftPanel();
    QWidget* setupRightPanel();

    std::unique_ptr<Grid> grid; // Grid nie jest QObject — trzymać w unique_ptr
    FlowModel* flowModel;       // zarządzanie przez Qt (parent = this)
    PaintTool* paintTool;       // zarządzanie przez Qt (parent = this)
    OpenGLRenderer* renderer;   // zarządzanie przez Qt (parent = this)
    QLabel* heightLabel;
};
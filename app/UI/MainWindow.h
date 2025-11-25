#pragma once
#include <QMainWindow>

class OpenGLRenderer;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
   public:
    explicit MainWindow(QWidget* parent = nullptr);

   private:
    OpenGLRenderer* renderer;
    QLabel* heightLabel;
};

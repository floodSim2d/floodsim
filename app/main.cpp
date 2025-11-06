#include <QApplication>
#include "UI/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Tworzymy główne okno programu
    MainWindow window;
    window.show();

    return app.exec();
}

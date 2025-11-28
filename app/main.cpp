#include <QApplication>
#include "UI/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    MainWindow w;
    w.showMaximized();

    return a.exec();
}

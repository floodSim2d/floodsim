#include <QApplication>
#include <QSurfaceFormat>
#include "UI/MainWindow.h"

int main(int argc, char *argv[]) {
    QSurfaceFormat format;
    format.setVersion(4,1);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    MainWindow window;
    window.showMaximized();

    return app.exec();
}

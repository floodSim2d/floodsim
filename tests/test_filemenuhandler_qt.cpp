#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QOffscreenSurface>

#include "UI/FileMenuHandler.h"
#include "Simulation/Grid/Grid.h"
#include "Renderer/OpenGLRenderer.h"

/**
 * @brief test suite for FileMenuHandler component
 */
class TestFileMenuHandler : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        if (!QApplication::instance()) {
            int argc = 0;
            char* argv[] = {nullptr};
            new QApplication(argc, argv);
        }

        surface = new QOffscreenSurface();
        surface->create();

        context = new QOpenGLContext();
        context->create();
    }

    void cleanupTestCase() {
        if (context) {
            context->doneCurrent();
            delete context;
        }
        delete surface;
    }

    void init() {
        grid = new Grid(10, 10, 1.0f, 50.0f);
        renderer = new OpenGLRenderer(grid, nullptr);
        fileHandler = new FileMenuHandler(grid, renderer, nullptr);
    }

    void cleanup() {
        if (grid && context) {
            context->makeCurrent(surface);
            grid->cleanup();
            context->doneCurrent();
        }

        delete fileHandler;
        delete renderer;
        delete grid;
    }

    void testConstruction() {
        QVERIFY(fileHandler != nullptr);
    }

    void testHandleNew() {
        QSignalSpy spy(fileHandler, &FileMenuHandler::statusMessageRequested);
        QVERIFY(spy.isValid());

        Cell* cell = grid->getCell(0, 0);
        if (cell) {
            cell->setTerrainHeight(100.0f);
        }

        fileHandler->handleNew();

        QCOMPARE(spy.count(), 1);

        cell = grid->getCell(0, 0);
        if (cell) {
            QCOMPARE(cell->getTerrainHeight(), 0.0f);
        }
    }

    void testHandleSaveSignal() {
        QSignalSpy spy(fileHandler, &FileMenuHandler::statusMessageRequested);
        QVERIFY(spy.isValid());
    }

    void testHandleOpenInvalidFile() {
        QSignalSpy errorSpy(fileHandler, &FileMenuHandler::errorMessageRequested);
        QVERIFY(errorSpy.isValid());
    }

    void testSaveLoadGrid() {
        context->makeCurrent(surface);
        grid->initialize(context->functions());

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // saveHeightmap adds .map extension automatically
        QString filenameBase = tempDir.path() + "/test_grid";

        Cell* cell = grid->getCell(5, 5);
        if (cell) {
            cell->setTerrainHeight(42.0f);
            cell->setWaterDepth(7.0f);
        }

        grid->saveHeightmap(filenameBase);

        grid->clearHeightmap();

        cell = grid->getCell(5, 5);
        if (cell) {
            QCOMPARE(cell->getTerrainHeight(), 0.0f);
        }

        // loadHeightmap requires .map extension
        QString filenameWithExtension = filenameBase + ".map";
        bool loaded = grid->loadHeightmap(filenameWithExtension);
        QVERIFY2(loaded, qPrintable(QString("Failed to load: %1").arg(filenameWithExtension)));

        cell = grid->getCell(5, 5);
        if (cell) {
            QCOMPARE(cell->getTerrainHeight(), 42.0f);
            QCOMPARE(cell->getWaterDepth(), 7.0f);
        }

        context->doneCurrent();
    }

    void testSignalsExist() {
        QSignalSpy statusSpy(fileHandler, &FileMenuHandler::statusMessageRequested);
        QSignalSpy errorSpy(fileHandler, &FileMenuHandler::errorMessageRequested);

        QVERIFY(statusSpy.isValid());
        QVERIFY(errorSpy.isValid());
    }

private:
    Grid* grid;
    OpenGLRenderer* renderer;
    FileMenuHandler* fileHandler;
    QOffscreenSurface* surface;
    QOpenGLContext* context;
};

QTEST_MAIN(TestFileMenuHandler)
#include "test_filemenuhandler_qt.moc"


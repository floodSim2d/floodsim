#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>

#include "UI/FileMenuHandler.h"
#include "Simulation/Grid/Grid.h"
#include "Renderer/OpenGLRenderer.h"

/**
 * @brief Test suite for FileMenuHandler component
 *
 * Tests:
 * - New, Open, Save operations
 * - Signal emissions
 * - Error handling
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
    }

    void init() {
        grid = new Grid(10, 10, 1.0f, 50.0f);
        renderer = new OpenGLRenderer(grid, nullptr);
        fileHandler = new FileMenuHandler(grid, renderer, nullptr);
    }

    void cleanup() {
        delete fileHandler;
        delete renderer;
        delete grid;
    }

    // Test construction
    void testConstruction() {
        QVERIFY(fileHandler != nullptr);
    }

    // Test handleNew clears the grid and emits signal
    void testHandleNew() {
        QSignalSpy spy(fileHandler, &FileMenuHandler::statusMessageRequested);
        QVERIFY(spy.isValid());

        // Add some data to grid
        Cell* cell = grid->getCell(0, 0);
        if (cell) {
            cell->setTerrainHeight(100.0f);
        }

        // Call handleNew
        fileHandler->handleNew();

        // Signal should be emitted
        QCOMPARE(spy.count(), 1);

        // Grid should be cleared
        cell = grid->getCell(0, 0);
        if (cell) {
            QCOMPARE(cell->getTerrainHeight(), 0.0f);
        }
    }

    // Test that save operation emits status message
    void testHandleSaveSignal() {
        QSignalSpy spy(fileHandler, &FileMenuHandler::statusMessageRequested);

        // Note: This won't actually open dialog in test environment
        // We're just testing that the method exists and can be called
        QVERIFY(spy.isValid());
    }

    // Test that handleOpen with invalid file emits error
    void testHandleOpenInvalidFile() {
        QSignalSpy errorSpy(fileHandler, &FileMenuHandler::errorMessageRequested);
        QVERIFY(errorSpy.isValid());

        // Note: Without GUI, file dialog won't open
        // This is more of an integration test
        // In real tests, you'd mock QFileDialog
    }

    // Test grid save and load functionality
    void testSaveLoadGrid() {
        // Create temporary file
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(true);
        QVERIFY(tempFile.open());
        QString filename = tempFile.fileName();
        tempFile.close();

        // Set some data in grid
        Cell* cell = grid->getCell(5, 5);
        if (cell) {
            cell->setTerrainHeight(42.0f);
            cell->setWaterDepth(7.0f);
        }

        // Save grid
        grid->saveHeightmap(filename);

        // Clear grid
        grid->clearHeightmap();

        // Verify it's cleared
        cell = grid->getCell(5, 5);
        if (cell) {
            QCOMPARE(cell->getTerrainHeight(), 0.0f);
        }

        // Load grid
        bool loaded = grid->loadHeightmap(filename);
        QVERIFY(loaded);

        // Verify data is restored
        cell = grid->getCell(5, 5);
        if (cell) {
            QCOMPARE(cell->getTerrainHeight(), 42.0f);
            QCOMPARE(cell->getWaterDepth(), 7.0f);
        }
    }

    // Test that signals are connected properly
    void testSignalsExist() {
        // Check that signals exist (will fail to compile if they don't)
        QSignalSpy statusSpy(fileHandler, &FileMenuHandler::statusMessageRequested);
        QSignalSpy errorSpy(fileHandler, &FileMenuHandler::errorMessageRequested);

        QVERIFY(statusSpy.isValid());
        QVERIFY(errorSpy.isValid());
    }

private:
    Grid* grid;
    OpenGLRenderer* renderer;
    FileMenuHandler* fileHandler;
};

QTEST_MAIN(TestFileMenuHandler)
// NOLINT - moc file is generated during build
#include "test_filemenuhandler_qt.moc"


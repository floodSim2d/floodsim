#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QAction>
#include <QLabel>

#include "UI/SimulationToolbar.h"
#include "Simulation/Grid/Grid.h"
#include "Simulation/Grid/Cell.h"
#include "Simulation/FlowModel/FlowModel.h"
#include "Renderer/OpenGLRenderer.h"

/**
 * @brief Test suite for SimulationToolbar component
 *
 * Tests:
 * - Control actions (Play, Pause, Step, Reset)
 * - Cell info display updates
 * - Signal emissions
 */
class TestSimulationToolbar : public QObject {
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
        flowModel = new FlowModel(grid, nullptr);
        renderer = new OpenGLRenderer(grid, nullptr);
        toolbar = new SimulationToolbar(grid, flowModel, renderer, nullptr);
    }

    void cleanup() {
        delete toolbar;
        delete renderer;
        delete flowModel;
        delete grid;
    }

    // Test construction
    void testConstruction() {
        QVERIFY(toolbar != nullptr);
    }

    // Test that toolbar has actions
    void testHasActions() {
        QList<QAction*> actions = toolbar->actions();
        QVERIFY(actions.size() > 0);
    }

    // Test that Play action exists and triggers simulation
    void testPlayAction() {
        QList<QAction*> actions = toolbar->actions();

        QAction* playAction = nullptr;
        for (QAction* action : actions) {
            if (action->text().contains("Start") || action->text().contains("▶")) {
                playAction = action;
                break;
            }
        }

        QVERIFY(playAction != nullptr);

        // Initially simulation should not be playing
        QVERIFY(!flowModel->isPlaying());

        // Trigger play action
        playAction->trigger();

        // Now simulation should be playing
        QVERIFY(flowModel->isPlaying());
    }

    // Test that Pause action exists and stops simulation
    void testPauseAction() {
        // Start simulation first
        flowModel->play();
        QVERIFY(flowModel->isPlaying());

        // Find pause action
        QList<QAction*> actions = toolbar->actions();
        QAction* pauseAction = nullptr;
        for (QAction* action : actions) {
            if (action->text().contains("Stop") || action->text().contains("⏸")) {
                pauseAction = action;
                break;
            }
        }

        QVERIFY(pauseAction != nullptr);

        // Trigger pause
        pauseAction->trigger();

        // Simulation should be paused
        QVERIFY(!flowModel->isPlaying());
    }

    // Test that Reset action exists and emits signal
    void testResetAction() {
        QSignalSpy spy(toolbar, &SimulationToolbar::statusMessageRequested);

        // Find reset action
        QList<QAction*> actions = toolbar->actions();
        QAction* resetAction = nullptr;
        for (QAction* action : actions) {
            if (action->text().contains("Reset")) {
                resetAction = action;
                break;
            }
        }

        QVERIFY(resetAction != nullptr);

        // Trigger reset
        resetAction->trigger();

        // Signal should be emitted
        QCOMPARE(spy.count(), 1);
    }

    // Test that Step action exists and emits signal
    void testStepAction() {
        QSignalSpy spy(toolbar, &SimulationToolbar::statusMessageRequested);

        // Find step action
        QList<QAction*> actions = toolbar->actions();
        QAction* stepAction = nullptr;
        for (QAction* action : actions) {
            if (action->text().contains("Krok") || action->text().contains("⏭")) {
                stepAction = action;
                break;
            }
        }

        QVERIFY(stepAction != nullptr);

        // Trigger step
        stepAction->trigger();

        // Signal should be emitted
        QCOMPARE(spy.count(), 1);
    }

    // Test that cell info label exists
    void testCellInfoLabel() {
        QLabel* label = toolbar->findChild<QLabel*>();
        QVERIFY(label != nullptr);
    }

    // Test cell info display updates when cell is hovered
    void testCellInfoUpdate() {
        QLabel* label = toolbar->findChild<QLabel*>();
        QVERIFY(label != nullptr);

        // Create a test cell
        Cell testCell;
        testCell.setTerrainHeight(10.0f);
        testCell.setWaterDepth(5.0f);

        // Simulate cell hover signal from renderer
        emit renderer->cellHovered(5, 5, testCell);

        // Process events to ensure signal is handled
        QTest::qWait(10);

        // Label should contain position and height info
        QString labelText = label->text();
        QVERIFY(labelText.contains("5"));  // Position
        QVERIFY(labelText.contains("10"));  // Height
    }

private:
    Grid* grid;
    FlowModel* flowModel;
    OpenGLRenderer* renderer;
    SimulationToolbar* toolbar;
};

QTEST_MAIN(TestSimulationToolbar)
// NOLINT - moc file is generated during build
#include "test_simulationtoolbar_qt.moc"


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
 * @brief test suite for SimulationToolbar component
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

    void testConstruction() {
        QVERIFY(toolbar != nullptr);
    }

    void testHasActions() {
        QList<QAction*> actions = toolbar->actions();
        QVERIFY(actions.size() > 0);
    }

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
        QVERIFY(!flowModel->isPlaying());

        playAction->trigger();

        QVERIFY(flowModel->isPlaying());
    }

    void testPauseAction() {
        flowModel->play();
        QVERIFY(flowModel->isPlaying());

        QList<QAction*> actions = toolbar->actions();
        QAction* pauseAction = nullptr;
        for (QAction* action : actions) {
            if (action->text().contains("Stop") || action->text().contains("⏸")) {
                pauseAction = action;
                break;
            }
        }

        QVERIFY(pauseAction != nullptr);

        pauseAction->trigger();

        QVERIFY(!flowModel->isPlaying());
    }

    void testResetAction() {
        QSignalSpy spy(toolbar, &SimulationToolbar::statusMessageRequested);

        QList<QAction*> actions = toolbar->actions();
        QAction* resetAction = nullptr;
        for (QAction* action : actions) {
            if (action->text().contains("Reset")) {
                resetAction = action;
                break;
            }
        }

        QVERIFY(resetAction != nullptr);

        resetAction->trigger();

        QCOMPARE(spy.count(), 1);
    }

    void testStepAction() {
        QSignalSpy spy(toolbar, &SimulationToolbar::statusMessageRequested);

        QList<QAction*> actions = toolbar->actions();
        QAction* stepAction = nullptr;
        for (QAction* action : actions) {
            if (action->text().contains("Krok") || action->text().contains("⏭")) {
                stepAction = action;
                break;
            }
        }

        QVERIFY(stepAction != nullptr);

        stepAction->trigger();

        QCOMPARE(spy.count(), 1);
    }

    void testCellInfoLabel() {
        QLabel* label = toolbar->findChild<QLabel*>();
        QVERIFY(label != nullptr);
    }

    void testCellInfoUpdate() {
        QLabel* label = toolbar->findChild<QLabel*>();
        QVERIFY(label != nullptr);

        Cell testCell;
        testCell.setTerrainHeight(10.0f);
        testCell.setWaterDepth(5.0f);

        emit renderer->cellHovered(5, 5, testCell);

        QTest::qWait(10);

        QString labelText = label->text();
        QVERIFY(labelText.contains("5"));
        QVERIFY(labelText.contains("10"));
    }

private:
    Grid* grid;
    FlowModel* flowModel;
    OpenGLRenderer* renderer;
    SimulationToolbar* toolbar;
};

QTEST_MAIN(TestSimulationToolbar)
#include "test_simulationtoolbar_qt.moc"


#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QPushButton>
#include <QSlider>

#include "UI/ToolPanel.h"
#include "Simulation/Tools/PaintTool.h"

/**
 * @brief test suite for ToolPanel component
 */
class TestToolPanel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
    }

    void cleanupTestCase() {
    }

    void init() {
        paintTool = new PaintTool(nullptr);
        toolPanel = new ToolPanel(paintTool, nullptr);
    }

    void cleanup() {
        delete toolPanel;
        delete paintTool;
    }

    void testConstruction() {
        QVERIFY(toolPanel != nullptr);
        QVERIFY(paintTool != nullptr);
    }

    void testBrushSizeSlider() {
        QSlider* slider = toolPanel->findChild<QSlider*>();
        QVERIFY(slider != nullptr);

        QCOMPARE(slider->value(), 1);
        QCOMPARE(paintTool->getBrushSize(), 1);

        slider->setValue(5);

        QCOMPARE(paintTool->getBrushSize(), 5);
    }

    void testToolSelectionSignal() {
        QSignalSpy spy(toolPanel, &ToolPanel::toolSelected);
        QVERIFY(spy.isValid());

        QList<QPushButton*> buttons = toolPanel->findChildren<QPushButton*>();
        QVERIFY(buttons.size() > 0);

        if (buttons.size() > 0) {
            buttons[0]->click();

            QCOMPARE(spy.count(), 1);

            QList<QVariant> arguments = spy.takeFirst();
            QVERIFY(arguments.at(0).toString().contains("Tryb kamery włączony - nawiguj sceną 3D"));
        }
    }

    void testCameraMode() {
        QSignalSpy spy(toolPanel, &ToolPanel::toolSelected);

        QList<QPushButton*> buttons = toolPanel->findChildren<QPushButton*>();

        if (buttons.size() > 0) {
            buttons[0]->click();
            QCOMPARE(spy.count(), 1);

            QList<QVariant> arguments = spy.takeFirst();
            QVERIFY(arguments.at(1).toBool() == true);
        }

        if (buttons.size() > 1) {
            buttons[1]->click();
            QCOMPARE(spy.count(), 1);

            QList<QVariant> arguments = spy.takeFirst();
            QVERIFY(arguments.at(1).toBool() == false); // not a camera mode
        }
    }

    void testToolButtonsCheckable() {
        QList<QPushButton*> buttons = toolPanel->findChildren<QPushButton*>();

        for (QPushButton* button : buttons) {
            // skip brush size value label
            if (button->text() != "1") {
                QVERIFY(button->isCheckable());
            }
        }
    }

private:
    PaintTool* paintTool;
    ToolPanel* toolPanel;
};

QTEST_MAIN(TestToolPanel)
#include "test_toolpanel_qt.moc"


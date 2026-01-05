#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QPushButton>
#include <QSlider>

#include "UI/ToolPanel.h"
#include "Simulation/Tools/PaintTool.h"

/**
 * @brief Test suite for ToolPanel component
 *
 * Tests:
 * - Tool selection buttons work correctly
 * - Brush size slider updates paint tool
 * - Signals are emitted properly
 */
class TestToolPanel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup before all tests
    }

    void cleanupTestCase() {
        // Cleanup after all tests
    }

    void init() {
        // Setup before each test
        paintTool = new PaintTool(nullptr);
        toolPanel = new ToolPanel(paintTool, nullptr);
    }

    void cleanup() {
        // Cleanup after each test
        delete toolPanel;
        delete paintTool;
    }

    // Test that ToolPanel is created successfully
    void testConstruction() {
        QVERIFY(toolPanel != nullptr);
        QVERIFY(paintTool != nullptr);
    }

    // Test that brush size slider updates paint tool
    void testBrushSizeSlider() {
        // Find the brush size slider
        QSlider* slider = toolPanel->findChild<QSlider*>();
        QVERIFY(slider != nullptr);

        // Initial value should be 1
        QCOMPARE(slider->value(), 1);
        QCOMPARE(paintTool->getBrushSize(), 1);

        // Change slider value
        slider->setValue(5);

        // PaintTool should be updated
        QCOMPARE(paintTool->getBrushSize(), 5);
    }

    // Test that tool selection emits signal
    void testToolSelectionSignal() {
        // Setup signal spy
        QSignalSpy spy(toolPanel, &ToolPanel::toolSelected);
        QVERIFY(spy.isValid());

        // Find all tool buttons
        QList<QPushButton*> buttons = toolPanel->findChildren<QPushButton*>();
        QVERIFY(buttons.size() > 0);

        // Click the first button (should be Camera)
        if (buttons.size() > 0) {
            buttons[0]->click();

            // Signal should have been emitted
            QCOMPARE(spy.count(), 1);

            // Check signal parameters
            QList<QVariant> arguments = spy.takeFirst();
            QVERIFY(arguments.at(0).toString().contains("Tryb kamery włączony - nawiguj sceną 3D"));
        }
    }

    // Test that camera mode is indicated correctly
    void testCameraMode() {
        QSignalSpy spy(toolPanel, &ToolPanel::toolSelected);

        QList<QPushButton*> buttons = toolPanel->findChildren<QPushButton*>();

        // First button should be Camera (isCameraMode = true)
        if (buttons.size() > 0) {
            buttons[0]->click();
            QCOMPARE(spy.count(), 1);

            QList<QVariant> arguments = spy.takeFirst();
            QVERIFY(arguments.at(1).toBool() == true);  // isCameraMode
        }

        // Second button should be Terrain (isCameraMode = false)
        if (buttons.size() > 1) {
            buttons[1]->click();
            QCOMPARE(spy.count(), 1);

            QList<QVariant> arguments = spy.takeFirst();
            QVERIFY(arguments.at(1).toBool() == false);  // not camera mode
        }
    }

    // Test that all tool buttons are checkable
    void testToolButtonsCheckable() {
        QList<QPushButton*> buttons = toolPanel->findChildren<QPushButton*>();

        for (QPushButton* button : buttons) {
            if (button->text() != "1") {  // Skip brush size value label
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


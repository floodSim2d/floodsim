#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>

#include "UI/ParameterPanel.h"
#include "Simulation/Grid/Grid.h"
#include "Simulation/FlowModel/FlowModel.h"
#include "Renderer/OpenGLRenderer.h"

/**
 * @brief Test suite for ParameterPanel component
 *
 * Tests:
 * - Parameter controls are created
 * - Apply button triggers signal
 * - Parameters are applied to FlowModel and Grid
 */
class TestParameterPanel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Create QApplication if not exists (needed for Qt GUI components)
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
        parameterPanel = new ParameterPanel(grid, flowModel, renderer, nullptr);
    }

    void cleanup() {
        delete parameterPanel;
        delete renderer;
        delete flowModel;
        delete grid;
    }

    // Test construction
    void testConstruction() {
        QVERIFY(parameterPanel != nullptr);
    }

    // Test that flow coefficient spin box exists
    void testFlowCoefficientSpinBox() {
        QDoubleSpinBox* spinBox = parameterPanel->findChild<QDoubleSpinBox*>();
        QVERIFY(spinBox != nullptr);
        QCOMPARE(spinBox->value(), 1.0);
    }

    // Test that max depth slider exists
    void testMaxDepthSlider() {
        QSlider* slider = parameterPanel->findChild<QSlider*>();
        QVERIFY(slider != nullptr);
        QVERIFY(slider->value() >= MIN_WATER_DEPTH);
        QVERIFY(slider->value() <= MAX_WATER_DEPTH);
    }

    // Test that apply button exists
    void testApplyButton() {
        QList<QPushButton*> buttons = parameterPanel->findChildren<QPushButton*>();
        bool foundApplyButton = false;

        for (QPushButton* button : buttons) {
            if (button->text().contains("Zastosuj")) {
                foundApplyButton = true;
                break;
            }
        }

        QVERIFY(foundApplyButton);
    }

    // Test that applying parameters emits signal
    void testApplySignal() {
        QSignalSpy spy(parameterPanel, &ParameterPanel::parametersApplied);
        QVERIFY(spy.isValid());

        // Find and click apply button
        QList<QPushButton*> buttons = parameterPanel->findChildren<QPushButton*>();
        for (QPushButton* button : buttons) {
            if (button->text().contains("Zastosuj")) {
                button->click();
                break;
            }
        }

        // Signal should be emitted
        QCOMPARE(spy.count(), 1);
    }

    // Test that parameters are actually applied to FlowModel
    void testParametersAppliedToFlowModel() {
        QDoubleSpinBox* spinBox = parameterPanel->findChild<QDoubleSpinBox*>();
        QVERIFY(spinBox != nullptr);

        // Set new flow coefficient
        spinBox->setValue(5.0);

        // Click apply
        QList<QPushButton*> buttons = parameterPanel->findChildren<QPushButton*>();
        for (QPushButton* button : buttons) {
            if (button->text().contains("Zastosuj")) {
                button->click();
                break;
            }
        }

        // Check that FlowModel was updated
        QCOMPARE(flowModel->getFlowCoefficient(), 5.0f);
    }

    // Test that max depth is applied to Grid
    void testMaxDepthAppliedToGrid() {
        QSlider* slider = parameterPanel->findChild<QSlider*>();
        QVERIFY(slider != nullptr);

        // Set new depth
        float newDepth = 100.0f;
        slider->setValue(static_cast<int>(newDepth));

        // Click apply
        QList<QPushButton*> buttons = parameterPanel->findChildren<QPushButton*>();
        for (QPushButton* button : buttons) {
            if (button->text().contains("Zastosuj")) {
                button->click();
                break;
            }
        }

        // Check that Grid was updated
        QCOMPARE(grid->getMaxDepth(), newDepth);
    }

private:
    Grid* grid;
    FlowModel* flowModel;
    OpenGLRenderer* renderer;
    ParameterPanel* parameterPanel;
};

QTEST_MAIN(TestParameterPanel)
// NOLINT - moc file is generated during build
#include "test_parameterpanel_qt.moc"


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
 * @brief test suite for ParameterPanel component
 */
class TestParameterPanel : public QObject {
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
        parameterPanel = new ParameterPanel(grid, flowModel, renderer, nullptr);
    }

    void cleanup() {
        delete parameterPanel;
        delete renderer;
        delete flowModel;
        delete grid;
    }

    void testConstruction() {
        QVERIFY(parameterPanel != nullptr);
    }

    void testFlowCoefficientSpinBox() {
        QDoubleSpinBox* spinBox = parameterPanel->findChild<QDoubleSpinBox*>();
        QVERIFY(spinBox != nullptr);
        QCOMPARE(spinBox->value(), 1.0);
    }

    void testMaxDepthSlider() {
        QSlider* slider = parameterPanel->findChild<QSlider*>();
        QVERIFY(slider != nullptr);
        QVERIFY(slider->value() >= World::MIN_WATER_DEPTH);
        QVERIFY(slider->value() <= World::MAX_WATER_DEPTH);
    }

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

    void testApplySignal() {
        QSignalSpy spy(parameterPanel, &ParameterPanel::parametersApplied);
        QVERIFY(spy.isValid());
        QList<QPushButton*> buttons = parameterPanel->findChildren<QPushButton*>();
        for (QPushButton* button : buttons) {
            if (button->text().contains("Zastosuj")) {
                button->click();
                break;
            }
        }

        QCOMPARE(spy.count(), 1);
    }

    void testParametersAppliedToFlowModel() {
        QDoubleSpinBox* spinBox = parameterPanel->findChild<QDoubleSpinBox*>();
        QVERIFY(spinBox != nullptr);

        spinBox->setValue(5.0);

        QList<QPushButton*> buttons = parameterPanel->findChildren<QPushButton*>();
        for (QPushButton* button : buttons) {
            if (button->text().contains("Zastosuj")) {
                button->click();
                break;
            }
        }

        QCOMPARE(flowModel->getPipeFriction(), 5.0f);
    }

    void testMaxDepthAppliedToGrid() {
        QSlider* slider = parameterPanel->findChild<QSlider*>();
        QVERIFY(slider != nullptr);

        float newDepth = 100.0f;
        slider->setValue(static_cast<int>(newDepth));

        QList<QPushButton*> buttons = parameterPanel->findChildren<QPushButton*>();
        for (QPushButton* button : buttons) {
            if (button->text().contains("Zastosuj")) {
                button->click();
                break;
            }
        }

        QCOMPARE(grid->getMaxDepth(), newDepth);
    }

private:
    Grid* grid;
    FlowModel* flowModel;
    OpenGLRenderer* renderer;
    ParameterPanel* parameterPanel;
};

QTEST_MAIN(TestParameterPanel)
#include "test_parameterpanel_qt.moc"


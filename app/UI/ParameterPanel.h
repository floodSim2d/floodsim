#ifndef FLOODSIM_PARAMETERPANEL_H
#define FLOODSIM_PARAMETERPANEL_H

#include <QWidget>

class Grid;
class FlowModel;
class OpenGLRenderer;
class QDoubleSpinBox;
class QSlider;
class QLabel;

/**
 * @brief right panel containing simulation parameters
 *
 * responsible for:
 * - flow coefficient (K) configuration
 * - max water depth configuration
 * - applying parameter changes
 */
class ParameterPanel : public QWidget {
    Q_OBJECT

public:
    explicit ParameterPanel(Grid* grid, FlowModel* flowModel, OpenGLRenderer* renderer, QWidget* parent = nullptr);
    ~ParameterPanel() override = default;

signals:
    /**
     * @brief emitted when parameters are applied
     * @param message status message to display
     */
    void parametersApplied(const QString& message);

private:
    void setupUI();
    void applyParameters();

    Grid* grid;
    FlowModel* flowModel;
    OpenGLRenderer* renderer;

    QDoubleSpinBox* flowCoefficientSpinBox;
    QSlider* maxDepthSlider;
    QLabel* depthValueLabel;
    QSlider* infiltrationSlider;
    QLabel* infiltrationValueLabel;
};

#endif // FLOODSIM_PARAMETERPANEL_H

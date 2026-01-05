#ifndef FLOODSIM_TOOLPANEL_H
#define FLOODSIM_TOOLPANEL_H

#include <QWidget>

class PaintTool;
class QLabel;
class QSlider;

/**
 * @brief left panel containing tool selection and brush size controls
 *
 * responsible for:
 * - displaying available painting tools
 * - managing tool selection
 * - controlling brush size
 */
class ToolPanel : public QWidget {
    Q_OBJECT

public:
    explicit ToolPanel(PaintTool* paintTool, QWidget* parent = nullptr);
    ~ToolPanel() override = default;

signals:
    /**
     * @brief user selects a tool
     * @param toolMessage message to display in status bar
     * @param isCameraMode true if camera tool is selected
     */
    void toolSelected(const QString& toolMessage, bool isCameraMode);

private:
    void setupUI();
    void setupToolButtons();
    void setupBrushControls();

    PaintTool* paintTool;
    QSlider* brushSizeSlider;
    QLabel* brushSizeValueLabel;
};

#endif // FLOODSIM_TOOLPANEL_H


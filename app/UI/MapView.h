#pragma once
#include <QWidget>
#include <memory>

class Grid;
class MapView : public QWidget {
    Q_OBJECT
public:
    explicit MapView(std::shared_ptr<Grid> grid, QWidget *parent = nullptr);

    enum class Tool {
        None,
        Terrain,
        Obstacle,
        River,
        WaterSource,
        Eraser
    };

    void setTool(Tool t) { currentTool = t; }
    void setBrushSize(int size) { m_brushSize = size; }


protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void applyToolAt(int x, int y);
    std::shared_ptr<Grid> m_grid;
    int cellSize = 10;
    int m_brushSize = 1;
    Tool currentTool = Tool::None;
};

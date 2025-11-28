#pragma once
#include <QWidget>
#include <vector>

class MapView : public QWidget {
    Q_OBJECT
public:
    explicit MapView(QWidget *parent = nullptr);

    enum class Tool {
        None,
        Terrain,
        Obstacle,
        River,
        WaterSource,
        Eraser
    };

    void setTool(Tool t) { currentTool = t; }

    bool saveToFile(const QString &path);
    bool loadFromFile(const QString &path);
    void clearMap();


protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Cell {
        int height = 0;
        bool obstacle = false;
        bool river = false;
        bool waterSource = false;
    };

    static constexpr int cellSize = 15;

    int gridW = 0;
    int gridH = 0;

    std::vector<Cell> grid;

    inline int index(int x, int y) const { return y * gridW + x; }

    Tool currentTool = Tool::None;

    void applyToolAt(int x, int y);
    void resizeGrid(int oldW, int oldH);
};

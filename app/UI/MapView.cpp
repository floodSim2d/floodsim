#include "MapView.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFile>
#include <QTextStream>
#include <QDataStream>

MapView::MapView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 200);
}

void MapView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    int oldW = gridW;
    int oldH = gridH;

    gridW = width() / cellSize +1;
    gridH = height() / cellSize +1;

    if (gridW != oldW || gridH != oldH)
        resizeGrid(oldW, oldH);

    update();
}

void MapView::resizeGrid(int oldW, int oldH) {

    std::vector<Cell> newGrid(gridW * gridH);

    if (!grid.empty()) {
        for (int y = 0; y < std::min(gridH, oldH); y++) {
            for (int x = 0; x < std::min(gridW, oldW); x++) {
                newGrid[y * gridW + x] = grid[y * oldW + x];
            }
        }
    }

    grid.swap(newGrid);
}

void MapView::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    p.fillRect(rect(), Qt::white);

    for (int y = 0; y < gridH; y++) {
        for (int x = 0; x < gridW; x++) {
            const Cell &c = grid[index(x, y)];
            QRect r(x * cellSize, y * cellSize, cellSize, cellSize);

            if (c.obstacle) {
                p.fillRect(r, Qt::black);
            }
            else if (c.river) {
                p.fillRect(r, QColor(0, 0, 255));
            }
            else if (c.waterSource) {
                p.fillRect(r, QColor(0, 255, 200));
            }
            else if (c.height > 0) {
                int brightness = 255 - (c.height * 15);
                if (brightness < 0) brightness = 0;
                p.fillRect(r, QColor(brightness, brightness, brightness));
            }
        }
    }

    p.setPen(QColor(200, 200, 200));
    for (int x = 0; x <= gridW; x++)
        p.drawLine(x * cellSize, 0, x * cellSize, gridH * cellSize);

    for (int y = 0; y <= gridH; y++)
        p.drawLine(0, y * cellSize, gridW * cellSize, y * cellSize);
}

void MapView::applyToolAt(int x, int y) {
    if (x < 0 || y < 0 || x >= gridW || y >= gridH)
        return;

    Cell &c = grid[index(x, y)];

    switch (currentTool) {
        case Tool::Terrain:
            c.height = std::min(c.height + 1, 20);
            break;
        case Tool::Obstacle:
            c.obstacle = true;
            break;
        case Tool::River:
            c.river = true;
            break;
        case Tool::WaterSource:
            c.waterSource = true;
            break;
        case Tool::Eraser:
            c.height = 0;
            c.obstacle = false;
            c.river = false;
            c.waterSource = false;
            break;
        default:
            break;
    }
}

void MapView::mousePressEvent(QMouseEvent *ev) {
    applyToolAt(ev->pos().x() / cellSize, ev->pos().y() / cellSize);
    update();
}

void MapView::mouseMoveEvent(QMouseEvent *ev) {
    if (ev->buttons() & Qt::LeftButton) {
        applyToolAt(ev->pos().x() / cellSize, ev->pos().y() / cellSize);
        update();
    }
}

bool MapView::saveToFile(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    QDataStream out(&f);

    out << gridW << gridH;

    for (const Cell &c : grid) {
        out << c.height
            << c.obstacle
            << c.river
            << c.waterSource;
    }

    return true;
}

bool MapView::loadFromFile(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    QDataStream in(&f);

    int w, h;
    in >> w >> h;

    if (w <= 0 || h <= 0)
        return false;

    gridW = w;
    gridH = h;
    grid.resize(gridW * gridH);

    for (Cell &c : grid) {
        in >> c.height
           >> c.obstacle
           >> c.river
           >> c.waterSource;
    }

    update();
    return true;
}

void MapView::clearMap() {
    for (Cell &c : grid) {
        c.height = 0;
        c.obstacle = false;
        c.river = false;
        c.waterSource = false;
    }
    update();
}

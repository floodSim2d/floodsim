#include "MapView.h"
#include "../Simulation/Grid.h" // Dołączamy nowy nagłówek
#include <QPainter>
#include <QMouseEvent>

MapView::MapView(std::shared_ptr<Grid> grid, QWidget *parent)
    : QWidget(parent), m_grid(grid)
{
    setMinimumSize(200, 200);
}

void MapView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    int newW = width() / cellSize + 1;
    int newH = height() / cellSize + 1;
    if (newW != m_grid->getWidth() || newH != m_grid->getHeight()) {
        m_grid->resize(newW, newH);
    }

    update();
}

void MapView::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    p.fillRect(rect(), Qt::white);

    const int gridW = m_grid->getWidth();
    const int gridH = m_grid->getHeight();

    for (int y = 0; y < gridH; y++) {
        for (int x = 0; x < gridW; x++) {
            const Cell &c = m_grid->getCell(x, y);
            QRect r(x * cellSize, y * cellSize, cellSize, cellSize);

            if (c.obstacle) {
                p.fillRect(r, Qt::black);
            } else if (c.river) {
                p.fillRect(r, QColor(0, 0, 255));
            } else if (c.waterSource) {
                p.fillRect(r, QColor(0, 255, 200));
            } else if (c.height > 0) {
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
    // Oblicz promień pędzla. Dla rozmiaru 1, promień = 0. Dla 3, promień = 1, itd.
    int radius = (m_brushSize - 1) / 2;

    // Iteruj po kwadratowym obszarze pędzla
    for (int brushY = -radius; brushY <= radius; ++brushY) {
        for (int brushX = -radius; brushX <= radius; ++brushX) {
            int currentX = x + brushX;
            int currentY = y + brushY;

            // Sprawdź, czy jesteśmy w granicach siatki
            if (currentX < 0 || currentY < 0 || currentX >= m_grid->getWidth() || currentY >= m_grid->getHeight())
                continue;

            Cell &c = m_grid->getCell(currentX, currentY);

            switch (currentTool) {
                case Tool::Terrain:
                    c.height = std::min(c.height + 1, 20);
                    break;
                case Tool::Obstacle: c.obstacle = true; break;
                case Tool::River: c.river = true; break;
                case Tool::WaterSource: c.waterSource = true; break;
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

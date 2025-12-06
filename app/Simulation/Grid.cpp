#include "Grid.h"
#include <QFile>
#include <QDataStream>
#include <algorithm>

Grid::Grid(int width, int height)
    : m_width(width), m_height(height), m_cells(width * height)
{
}

void Grid::resize(int newW, int newH) {
    std::vector<Cell> newGrid(newW * newH);

    if (!m_cells.empty()) {
        for (int y = 0; y < std::min(m_height, newH); y++) {
            for (int x = 0; x < std::min(m_width, newW); x++) {
                newGrid[y * newW + x] = getCell(x, y);
            }
        }
    }

    m_width = newW;
    m_height = newH;
    m_cells.swap(newGrid);
}

Cell& Grid::getCell(int x, int y) {
    return m_cells[index(x, y)];
}

const Cell& Grid::getCell(int x, int y) const {
    return m_cells[index(x, y)];
}

int Grid::index(int x, int y) const {
    return y * m_width + x;
}

void Grid::clear() {
    for (Cell &c : m_cells) {
        c.water_depth = 0.0f;
        c.height = 0;
        c.obstacle = false;
        c.river = false;
        c.waterSource = false;
    }
}

bool Grid::saveToFile(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    QDataStream out(&f);
    out << m_width << m_height;

    for (const Cell &c : m_cells) {
        out << c.height
            << c.water_depth
            << c.obstacle
            << c.river
            << c.waterSource;
    }
    return true;
}

bool Grid::loadFromFile(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    QDataStream in(&f);
    int w, h;
    in >> w >> h;

    if (w <= 0 || h <= 0)
        return false;

    m_width = w;
    m_height = h;
    m_cells.resize(m_width * m_height);

    for (Cell &c : m_cells) {
        in >> c.height
           >> c.water_depth
           >> c.obstacle
           >> c.river
           >> c.waterSource;
    }
    return true;
}
#pragma once

#include <vector>
#include <QString>
#include "../Data/Cell.h"

class Grid
{
public:
    Grid(int width = 1, int height = 1);

    void resize(int newW, int newH);
    void clear();

    bool saveToFile(const QString &path);
    bool loadFromFile(const QString &path);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    Cell& getCell(int x, int y);
    const Cell& getCell(int x, int y) const;

private:
    int m_width;
    int m_height;
    std::vector<Cell> m_cells;

    int index(int x, int y) const;
};
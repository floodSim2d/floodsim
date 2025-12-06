#pragma once
struct Cell {
    int height = 0;
    float water_depth = 0.0f;
    bool obstacle = false;
    bool river = false;
    bool waterSource = false;
};
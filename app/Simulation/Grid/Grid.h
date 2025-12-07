#ifndef FLOODSIM_GRID_H
#define FLOODSIM_GRID_H

#include <QOpenGLFunctions>

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QVector2D>
#include <vector>

#include "Cell.h"

class Grid {
   public:
    Grid(int width, int height, float cellSize);
    ~Grid();

    void initialize(QOpenGLFunctions* gl_context);
    void render(const QMatrix4x4& projection, const QMatrix4x4& view) const;

    // getters setters
    Cell* getCell(int x, int y);
    void setCell(int x, int y, const Cell& value);

    auto getWidth() const { return width; }
    auto getHeight() const { return height; }
    auto getCellSize() const { return cellSize; }

    void saveHeightmap(const QString& filename) const;
    void loadHeightmap(const QString& filename);
    void clearHeightmap(const Cell& defaultCell = Cell());
    void updateHeightTexture() const;

    // utils
    bool isValidPosition(int x, int y) const;
    QVector2D worldPosToGrid(const QVector2D& worldPos) const;

   private:
    void createMesh();
    void createHeightTexture();
    void createShaders();

    // opengl
    QOpenGLFunctions* glContext;
    QOpenGLShaderProgram* shaderProgram;
    QOpenGLBuffer* vertexBuffer;  // VBO
    QOpenGLBuffer* indexBuffer;   // EBO
    QOpenGLVertexArrayObject* VAO;
    GLuint texture = 0;

    // grid data
    unsigned int width;
    unsigned int height;
    float cellSize;
    std::vector<Cell> heightMap;

    // rendering data
    unsigned int indexCount;
    unsigned int meshResolution;
};

#endif  // FLOODSIM_GRID_H
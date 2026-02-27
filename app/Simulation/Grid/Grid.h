#ifndef FLOODSIM_GRID_H
#define FLOODSIM_GRID_H

#include <QOpenGLFunctions>

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QVector2D>
#include <vector>

#include "Cell.h"

constexpr float DEFAULT_WATER_DEPTH = 50.0F;
constexpr float MAX_WATER_DEPTH = 200.0F;
constexpr float MIN_WATER_DEPTH = 10.0F;

class Grid {
   public:
    Grid(int width, int height, float cellSize, float maxDepth = 10.0F);
    ~Grid();

    void initialize(QOpenGLFunctions* gl_context);
    void cleanup();
    void render(const QMatrix4x4& projection, const QMatrix4x4& view) const;

    // getters setters
    Cell* getCell(int x, int y);
    const Cell* getCell(int x, int y) const;
    void setCell(int x, int y, const Cell& value);

    auto getWidth() const { return width; }
    auto getHeight() const { return height; }
    auto getCellSize() const { return cellSize; }
    auto getMaxDepth() const { return maxDepth; }
    void setMaxDepth(const float depth) { maxDepth = depth; }

    void saveHeightmap(const QString& filename) const;
    [[nodiscard]] bool loadHeightmap(const QString& filename);
    void clearHeightmap(const Cell& defaultCell = Cell());
    void updateHeightTexture();

    // utils
    [[nodiscard]] bool isValidPosition(int x, int y) const;
    [[nodiscard]] QVector2D worldPosToGrid(const QVector2D& worldPos) const;

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
    QOpenGLTexture* heightTexture;

    // PBO for async texture uploads (double buffering)
    QOpenGLBuffer* pbo[2];
    int currentPboIndex;
    size_t pboSize;

    void createPBOs();
    void destroyPBOs();

    void rollbackToSize(unsigned int originalWidth, unsigned int originalHeight, bool recreateTexture);

    // grid data
    unsigned int width;
    unsigned int height;
    float cellSize;
    float maxDepth;
    std::vector<Cell> heightMap;

    // rendering data
    unsigned int indexCount;
    unsigned int meshResolution;
};

#endif  // FLOODSIM_GRID_H
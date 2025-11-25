#ifndef FLOODSIM_GRID_H
#define FLOODSIM_GRID_H

#include <qopenglfunctions_3_3_core.h>

#include <QFile>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QVector2D>
#include <vector>

class Grid {
   public:
    Grid(int width, int height, float cellSize);
    ~Grid();

    void initialize(QOpenGLFunctions_3_3_Core* glContext);
    void render(const QMatrix4x4 projection, const QMatrix4x4 view);

    // getters setters
    float getHeightValue(int x, int y) const;
    void setHeightValue(int x, int y, float height);

    auto getWidth() const { return width; }
    auto getHeight() const { return height; }
    auto getCellSize() const { return cellSize; }

    void saveHeightmap(const QString& filename) const;
    void loadHeightmap(const QString& filename);
    void clearHeightmap(float value = 0.0f);
    void updateHeightTexture();

    // utils
    bool isValidPosition(int x, int y) const;
    QVector2D worldPosToGrid(const QVector2D& worldPos) const;

   private:
    void createMesh();
    void createHeightTexture();
    void createShaders();

    // opengl
    QOpenGLFunctions_3_3_Core* glContext;
    QOpenGLShaderProgram* shaderProgram;
    QOpenGLBuffer* vertexBuffer;  // VBO
    QOpenGLBuffer* indexBuffer;   // EBO
    QOpenGLVertexArrayObject* VAO;
    QOpenGLTexture* heightTexture;

    // grid data
    unsigned int width;
    unsigned int height;
    float cellSize;
    std::vector<float> heightMap;

    // rendering data
    unsigned int indexCount;
    unsigned int meshResolution;
};

#endif  // FLOODSIM_GRID_H
#ifndef FLOODSIM_WATERRENDERER_H
#define FLOODSIM_WATERRENDERER_H

#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

class Grid;

/**
 * @brief Renders water as a separate translucent surface above terrain.
 *
 * The water mesh is positioned at (terrainHeight + waterDepth) for each cell,
 * creating a proper water surface that floats above the terrain.
 * Rendered with alpha blending AFTER the terrain pass (depth-write disabled).
 *
 * Uses static procedural texture (Voronoi cells + particle specks),
 * no time-based animation.
 */
class WaterRenderer {
public:
    explicit WaterRenderer(Grid* grid);
    ~WaterRenderer();

    void initialize(QOpenGLFunctions* glContext);
    void cleanup();

    /**
     * @brief Renders the water surface.
     *
     * Must be called after terrain rendering with blending enabled
     * and depth-write disabled (glDepthMask(GL_FALSE)).
     */
    void render(const QMatrix4x4& projection, const QMatrix4x4& view,
                QOpenGLTexture* heightMapTexture, const QVector3D& viewPos) const;

private:
    void createShaders();
    void createMesh();

    Grid* grid;
    QOpenGLFunctions* glContext;

    QOpenGLShaderProgram* shaderProgram;
    QOpenGLBuffer* vertexBuffer;
    QOpenGLBuffer* indexBuffer;
    QOpenGLVertexArrayObject* VAO;

    unsigned int indexCount;
    unsigned int meshResolution;
};

#endif // FLOODSIM_WATERRENDERER_H


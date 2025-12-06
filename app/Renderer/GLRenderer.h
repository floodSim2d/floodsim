#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <memory>

class Grid;

class GLRenderer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit GLRenderer(std::shared_ptr<Grid> grid, QWidget *parent = nullptr);
    ~GLRenderer() override;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    void setupShaders();
    void setupGeometry();
    void setupTextures();
    void updateTerrainTexture();

    std::shared_ptr<Grid> m_grid;

    QOpenGLShaderProgram* m_terrainProgram = nullptr;
    QOpenGLVertexArrayObject m_quadVAO;
    QOpenGLBuffer m_quadVBO;
    GLuint m_terrainTextureID = 0;
};

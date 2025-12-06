#ifndef FLOODSIM_OPENGLRENDERER_H
#define FLOODSIM_OPENGLRENDERER_H

#include <QMatrix4x4>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QTimer>
#include <QVector3D>
#include <memory>

class Grid;

class OpenGLRenderer : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

   public:
    explicit OpenGLRenderer(QWidget* parent = nullptr);
    ~OpenGLRenderer() override;

    // Grid access
    [[nodiscard]] Grid* getGrid() const { return grid.get(); }

    // Camera control
    void setZoom(float zoom);
    void panCamera(float deltaX, float deltaY);
    void resetCamera();

   signals:
    void heightValueHovered(int gridX, int gridY, float height);
    void cellClicked(int gridX, int gridY);

   protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

   private:
    void setupCamera();
    bool screenToGridCoords(int screenX, int screenY, int& gridX, int& gridY) const;

    // Grid
    std::unique_ptr<Grid> grid;

    // Camera/View matrices
    QMatrix4x4 projectionMatrix;
    QMatrix4x4 viewMatrix;

    // Camera state
    float cameraZoom;
    QVector3D cameraPosition;
    QVector3D cameraTarget;

    // Mouse interaction
    QPoint lastMousePos;
    bool isDragging;
    int hoveredGridX;
    int hoveredGridY;
};

#endif  // FLOODSIM_OPENGLRENDERER_H

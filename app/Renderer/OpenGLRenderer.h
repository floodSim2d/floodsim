#ifndef FLOODSIM_OPENGLRENDERER_H
#define FLOODSIM_OPENGLRENDERER_H

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QTimer>
#include <QVector3D>
#include <memory>
#include <QString>

#include "../Simulation/Tools/PaintTool.h"

constexpr float CAMERA_ZOOM_MAX_ORTHO = 200.0F;
constexpr float CAMERA_ZOOM_MIN_ORTHO = 5.0F;
constexpr float CAMERA_ZOOM_MAX_PERSP = 200.0F;
constexpr float CAMERA_ZOOM_MIN_PERSP = 5.0F;
constexpr float CAMERA_MAX_HEIGHT = 1000.0F;

class Cell;
class Grid;

enum class CameraMode {
    TopDown, // Orthographic, for editing
    Orbit    // Perspective, for navigation
};

class OpenGLRenderer : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit OpenGLRenderer(QWidget* parent = nullptr);
    ~OpenGLRenderer() override;

    // Grid access
    Grid* getGrid() const { return grid.get(); }

    // Camera control
    void setCameraMode(CameraMode mode);
    CameraMode getCameraMode() const { return cameraMode; }
    void resetCamera();
    void setCameraPanEnabled(bool enabled); // Kept for compatibility with MainWindow logic
    bool isCameraPanEnabled() const;


    // Paint tool
    auto getPaintTool() const -> PaintTool* { return paintTool; }

signals:
    void cellHovered(int gridX, int gridY, const Cell& cell);
    void cellClicked(int gridX, int gridY);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void setupCamera();
    void updateProjectionMatrix();
    bool screenToGridCoords(int screenX, int screenY, int& gridX, int& gridY) const;

    // Camera actions
    void panCamera(float deltaX, float deltaY);
    void rotateCamera(float yawDelta, float pitchDelta);
    void moveCameraVertical(float delta);
    void setZoom(float zoom);

    // Grid
    std::unique_ptr<Grid> grid;

    // Camera/View matrices
    QMatrix4x4 projectionMatrix;
    QMatrix4x4 viewMatrix;

    // Camera state
    CameraMode cameraMode;
    float cameraZoom;
    QVector3D cameraPosition;
    QVector3D cameraTarget;
    float cameraYaw;
    float cameraPitch;

    // Mouse interaction
    QPoint lastMousePos;
    bool isDragging;
    int hoveredGridX;
    int hoveredGridY;

    // Paint tool
    PaintTool* paintTool;
};

#endif  // FLOODSIM_OPENGLRENDERER_H

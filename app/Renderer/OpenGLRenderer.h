#ifndef FLOODSIM_OPENGLRENDERER_H
#define FLOODSIM_OPENGLRENDERER_H

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QVector3D>
#include <QString>

#include "../Simulation/Tools/PaintTool.h"
#include "WaterRenderer.h"

constexpr float CAMERA_ZOOM_MAX_ORTHO = 100.0F;
constexpr float CAMERA_ZOOM_MIN_ORTHO = 10.0F;
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
    explicit OpenGLRenderer(Grid* grid,WaterRenderer* water_renderer ,QWidget* parent = nullptr);
    ~OpenGLRenderer() override;

    auto getGrid() const -> Grid* { return grid; }
    auto getWaterRenderer() const -> WaterRenderer* { return waterRenderer; }

    // Camera control
    void setCameraMode(CameraMode mode);
    CameraMode getCameraMode() const { return cameraMode; }
    void setZoom(float zoom);
    void panCamera(float deltaX, float deltaY);
    void resetCamera();
    void setCameraPanEnabled(bool enabled);
    bool isCameraPanEnabled() const { return cameraMode == CameraMode::Orbit; }
    void updateProjectionMatrix();

    // Paint tool
    void setPaintTool(PaintTool* tool) { paintTool = tool; }
    auto getPaintTool() const -> PaintTool* { return paintTool; }

   signals:
    void cellHovered(int gridX, int gridY, const Cell& cell);
    void cellClicked(int gridX, int gridY);
    void cameraPanToggled(bool enabled);

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
    void rotateCamera(float yawDelta, float pitchDelta);
    void moveCameraVertical(float delta);
    bool screenToGridCoords(int screenX, int screenY, int& gridX, int& gridY) const;

    Grid* grid;

    WaterRenderer* waterRenderer;

    // Camera/View matrices
    QMatrix4x4 projectionMatrix;
    QMatrix4x4 viewMatrix;

    // Camera state
    CameraMode cameraMode;
    float cameraZoom;
    QVector3D cameraPosition;
    QVector3D cameraTarget;
    float cameraYaw;      // For Orbit mode
    float cameraPitch;    // For Orbit mode

    // Mouse interaction
    QPoint lastMousePos;
    bool isDragging;
    int hoveredGridX;
    int hoveredGridY;

    // Paint tool
    PaintTool* paintTool;
    bool cameraPanEnabled;
};

#endif  // FLOODSIM_OPENGLRENDERER_H
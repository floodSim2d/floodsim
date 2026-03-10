#ifndef FLOODSIM_OPENGLRENDERER_H
#define FLOODSIM_OPENGLRENDERER_H

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QVector3D>
#include <QString>

#include "../Simulation/Tools/PaintTool.h"
#include "../WorldConstants.h"
#include "WaterRenderer.h"


class Cell;
class Grid;

enum class CameraMode {
    TopDown, // Orthographic, for editing
    Orbit    // Perspective, for navigation
};

class OpenGLRenderer : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

   public:
    explicit OpenGLRenderer(Grid* grid, WaterRenderer* water_renderer, QWidget* parent = nullptr);
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

    // Google-Maps-style navigation helpers
    // Unprojects a screen pixel onto the ground plane (Z=0) in world space.
    // Returns false if the ray is parallel to or points away from the ground.
    bool screenToGroundPlane(int screenX, int screenY, QVector3D& worldPos) const;

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
    float cameraYaw;      // Orbit mode: horizontal angle (degrees)
    float cameraPitch;    // Orbit mode: vertical tilt (degrees, negative = looking down)

    // Mouse interaction
    QPoint lastMousePos;
    bool isDragging;
    int hoveredGridX;
    int hoveredGridY;

    // Google-Maps navigation state
    QVector3D orbitFocusPoint;   // world-space point around which we orbit (under cursor at drag-start)
    QVector3D panAnchorWorld;    // world-space point "glued" to cursor during pan
    bool isPanning;              // true while PPM is held in Orbit mode
    bool isOrbiting;             // true while LPM is held in Orbit mode

    // Paint tool
    PaintTool* paintTool;
    bool cameraPanEnabled;
};

#endif  // FLOODSIM_OPENGLRENDERER_H

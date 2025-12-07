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

class Cell;
class Grid;

class OpenGLRenderer : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

   public:
    explicit OpenGLRenderer(QWidget* parent = nullptr);
    ~OpenGLRenderer() override;

    // Grid access
    Grid* getGrid() const { return grid.get(); }

    // Camera control
    void setZoom(float zoom);
    void panCamera(float deltaX, float deltaY);
    void resetCamera();
    void setCameraPanEnabled(bool enabled);
    bool isCameraPanEnabled() const { return cameraPanEnabled; }

    // Paint tool
    void setToolType(ToolType tool);
    ToolType getCurrentToolType() const;
    void setBrushSize(int size);
    int getBrushSize() const;

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
    void wheelEvent(QWheelEvent* event) override;

   private:
    void setupCamera();
    void updateProjectionMatrix();
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

    // Paint tool
    PaintTool paintTool;
    bool cameraPanEnabled;
};

#endif  // FLOODSIM_OPENGLRENDERER_H

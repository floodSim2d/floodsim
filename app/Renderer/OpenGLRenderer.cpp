#include "OpenGLRenderer.h"

#include <QWheelEvent>
#include <QtMath>
#include <cmath>

#include "../Simulation/Grid/Grid.h"
#include "../Simulation/Grid/Cell.h"

OpenGLRenderer::OpenGLRenderer(QWidget* parent)
    : QOpenGLWidget(parent),
      grid(nullptr),
      cameraZoom(50.0f), // Initial zoom/distance for perspective
      cameraPosition(0.0F, 0.0F, CAMERA_MAX_HEIGHT),
      cameraTarget(0.0F, 0.0F, 0.0F),
      isDragging(false),
      hoveredGridX(-1),
      hoveredGridY(-1),
      paintTool(new PaintTool(this)),
      cameraPanEnabled(false) {
    setMouseTracking(true);

    connect(paintTool, &PaintTool::paintApplied, this, QOverload<>::of(&QWidget::update));
}

OpenGLRenderer::~OpenGLRenderer() {
    makeCurrent();
    grid.reset();
    doneCurrent();
}

void OpenGLRenderer::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.15F, 0.15F, 0.15F, 1.0F); // Dark gray background
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    qDebug() << "OpenGL Renderer initialized.";
    qDebug() << "Vendor:" << reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    qDebug() << "Renderer:" << reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    qDebug() << "Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));

    grid = std::make_unique<Grid>(200, 200, 1.0F, DEFAULT_WATER_DEPTH);
    grid->initialize(this);

    // Center camera on grid - position directly above for top-down view
    const float gridCenterX = static_cast<float>(grid->getWidth()) * grid->getCellSize() * 0.5F;
    const float gridCenterY = static_cast<float>(grid->getHeight()) * grid->getCellSize() * 0.5F;
    cameraTarget = QVector3D(gridCenterX, gridCenterY, 0.0f);
    // Start with a perspective view, not directly top-down
    const float initialHeight = std::max(grid->getWidth(), grid->getHeight()) * grid->getCellSize();
    cameraPosition = QVector3D(gridCenterX, gridCenterY - initialHeight * 0.7f, initialHeight);

    setupCamera();
}

void OpenGLRenderer::resizeGL(const int width, const int height) {
    glViewport(0, 0, width, height);
    updateProjectionMatrix();
}

void OpenGLRenderer::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // uncomment for testing wireframe mode
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if (grid) {
        grid->render(projectionMatrix, viewMatrix);
    }
}

void OpenGLRenderer::setupCamera() {
    viewMatrix.setToIdentity();
    viewMatrix.lookAt(cameraPosition, cameraTarget, QVector3D(0.0F, 1.0F, 0.0F));
}

void OpenGLRenderer::updateProjectionMatrix() {
    const float aspect = static_cast<float>(width()) / static_cast<float>(height());
    projectionMatrix.setToIdentity();

    // Use perspective projection for 3D camera movement
    const float fieldOfView = 45.0f;
    const float nearPlane = 0.1F;
    const float farPlane = CAMERA_MAX_HEIGHT * 2.0f; // A large value to see everything

    projectionMatrix.perspective(fieldOfView, aspect, nearPlane, farPlane);

    // Note: cameraZoom is now used for wheel-based zooming (dolly)
    // and is handled in wheelEvent by moving the camera position.
}


void OpenGLRenderer::setZoom(const float zoom) {
    cameraZoom = std::max(CAMERA_ZOOM_MIN, std::min(CAMERA_ZOOM_MAX, zoom));

    updateProjectionMatrix();
    update();
}

void OpenGLRenderer::setCameraPanEnabled(bool enabled) {
    cameraPanEnabled = enabled;
    emit cameraPanToggled(enabled);
}

void OpenGLRenderer::resetCamera() {
    if (!grid) {
        return;
    }

    float gridCenterX = grid->getWidth() * grid->getCellSize() * 0.5F;
    float gridCenterY = grid->getHeight() * grid->getCellSize() * 0.5F;

    cameraTarget = QVector3D(gridCenterX, gridCenterY, 0.0F);

    // Reset to the initial perspective view
    const float initialHeight = std::max(grid->getWidth(), grid->getHeight()) * grid->getCellSize();
    cameraPosition = QVector3D(gridCenterX, gridCenterY - initialHeight * 0.7f, initialHeight);
    cameraZoom = 50.0f; // Reset zoom value

    updateProjectionMatrix();
    setupCamera();
    update();
}

void OpenGLRenderer::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
        isDragging = true;

        int gridX;
        int gridY;
        if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
            // If paint tool is active, start continuous painting
            if (paintTool->getToolType() != ToolType::Camera) {
                paintTool->startContinuousPainting(grid.get(), gridX, gridY);
            }
            emit cellClicked(gridX, gridY);
        }
    }

    QOpenGLWidget::mousePressEvent(event);
}

void OpenGLRenderer::mouseMoveEvent(QMouseEvent* event) {
    // Handle dragging with left button
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        const QPoint delta = event->pos() - lastMousePos;
        lastMousePos = event->pos();

        if (cameraPanEnabled) {
            rotateCamera(static_cast<float>(delta.x()));
            moveCameraUpDown(static_cast<float>(delta.y()));
        } else if (paintTool->getToolType() != ToolType::Camera) {
            // Paint mode - update current position for continuous painting
            int gridX, gridY;
            if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                paintTool->updatePaintPosition(gridX, gridY);
            }
        }
    }

    // Handle hover - show cell label
    int gridX;
    int gridY;
    if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
        if (gridX != hoveredGridX || gridY != hoveredGridY) {
            hoveredGridX = gridX;
            hoveredGridY = gridY;

            const auto* cell = grid->getCell(gridX, gridY);
            if (cell != nullptr) {
                emit cellHovered(gridX, gridY, *cell);
            }
        }
    } else {
        hoveredGridX = -1;
        hoveredGridY = -1;
    }

    QOpenGLWidget::mouseMoveEvent(event);
}

void OpenGLRenderer::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        paintTool->stopContinuousPainting();
    }

    QOpenGLWidget::mouseReleaseEvent(event);
}

void OpenGLRenderer::wheelEvent(QWheelEvent* event) {
    const float delta = event->angleDelta().y() / 120.0F;

    // Dolly zoom: move camera forward/backward along its view direction
    QVector3D viewDir = (cameraTarget - cameraPosition).normalized();
    float zoomSpeed = cameraPosition.distanceToPoint(cameraTarget) * 0.1f;
    zoomSpeed = std::max(0.5f, zoomSpeed); // Ensure a minimum zoom speed

    cameraPosition += viewDir * delta * zoomSpeed;

    setupCamera();
    update();

    event->accept();
}

bool OpenGLRenderer::screenToGridCoords(const int screenX, const int screenY, int& gridX, int& gridY) const {
    if (!grid)
        return false;

    // 1. Screen space to NDC
    const float ndcX = (2.0f * screenX) / width() - 1.0f;
    const float ndcY = 1.0f - (2.0f * screenY) / height();

    // 2. NDC to Camera (View) space
    const QMatrix4x4 invProj = projectionMatrix.inverted();
    QVector4D ray_eye = invProj * QVector4D(ndcX, ndcY, -1.0, 1.0);
    ray_eye.setZ(-1.0); // Point into the screen
    ray_eye.setW(0.0);  // This is a direction vector

    // 3. Camera (View) space to World space
    const QMatrix4x4 invView = viewMatrix.inverted();
    QVector3D ray_world = (invView * ray_eye).toVector3D().normalized();
    QVector3D camera_world_pos = (invView * QVector4D(0, 0, 0, 1)).toVector3D();

    // 4. Ray-Plane Intersection (plane is at z=0)
    QVector3D planeNormal(0.0f, 0.0f, 1.0f);
    QVector3D planePoint(0.0f, 0.0f, 0.0f); // Any point on the z=0 plane

    float denom = QVector3D::dotProduct(ray_world, planeNormal);

    // Check if ray is parallel to the plane
    if (std::abs(denom) > 1e-6) {
        float t = QVector3D::dotProduct(planePoint - camera_world_pos, planeNormal) / denom;
        if (t >= 0) { // Intersection is in front of the camera
            QVector3D worldPos = camera_world_pos + t * ray_world;
            const float cellSize = grid->getCellSize();
            gridX = static_cast<int>(floor(worldPos.x() / cellSize));
            gridY = static_cast<int>(floor(worldPos.y() / cellSize));
            return grid->isValidPosition(gridX, gridY);
        }
    }

    return false;
}

void OpenGLRenderer::rotateCamera(float deltaX) {
    float angle = deltaX * 0.4f; // Rotation sensitivity
    QVector3D viewDir = cameraPosition - cameraTarget;
    QMatrix4x4 rotationMatrix;
    rotationMatrix.rotate(angle, 0.0f, 0.0f, 1.0f); // Rotate around world Z-axis for "up"
    cameraPosition = cameraTarget + rotationMatrix * viewDir;
    setupCamera();
    update();
}

void OpenGLRenderer::moveCameraUpDown(float deltaY) {
    float moveSpeed = 0.5f; // Up/down movement sensitivity
    cameraPosition.setZ(cameraPosition.z() - deltaY * moveSpeed);
    setupCamera();
    update();
}

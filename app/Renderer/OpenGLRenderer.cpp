#include "OpenGLRenderer.h"

#include <QWheelEvent>
#include <QtMath>
#include <cmath>

#include "../Simulation/Grid/Grid.h"
#include "../Simulation/Grid/Cell.h"

OpenGLRenderer::OpenGLRenderer(QWidget* parent)
    : QOpenGLWidget(parent),
      grid(nullptr),
      cameraMode(CameraMode::Orbit), // Zmieniono domyślny tryb
      cameraZoom(150.0f),
      cameraTarget(0.0F, 0.0F, 0.0F),
      cameraYaw(-90.0f),
      cameraPitch(-30.0f),
      isDragging(false),
      hoveredGridX(-1),
      hoveredGridY(-1),
      paintTool(new PaintTool(this)) {
    setMouseTracking(true);
    connect(paintTool, &PaintTool::paintApplied, this, QOverload<>::of(&QWidget::update));
}

OpenGLRenderer::~OpenGLRenderer() {
    makeCurrent();
    grid.reset();
    delete paintTool;
    doneCurrent();
}

void OpenGLRenderer::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.15F, 0.15F, 0.15F, 1.0F);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    grid = std::make_unique<Grid>(200, 200, 1.0F, DEFAULT_WATER_DEPTH);
    grid->initialize(this);

    resetCamera(); // Wystarczy reset, bo domyślny tryb jest już ustawiony
}

void OpenGLRenderer::resizeGL(const int width, const int height) {
    glViewport(0, 0, width, height);
    updateProjectionMatrix();
}

void OpenGLRenderer::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (grid) {
        grid->render(projectionMatrix, viewMatrix);
    }
}

void OpenGLRenderer::setCameraMode(CameraMode mode) {
    if (cameraMode == mode) return;
    cameraMode = mode;
    resetCamera();
}

void OpenGLRenderer::setCameraPanEnabled(bool enabled) {
    setCameraMode(enabled ? CameraMode::Orbit : CameraMode::TopDown);
}

bool OpenGLRenderer::isCameraPanEnabled() const {
    return cameraMode == CameraMode::Orbit;
}

void OpenGLRenderer::resetCamera() {
    if (!grid) return;

    float gridCenterX = grid->getWidth() * grid->getCellSize() * 0.5F;
    float gridCenterY = grid->getHeight() * grid->getCellSize() * 0.5F;

    if (cameraMode == CameraMode::TopDown) {
        cameraPosition = QVector3D(gridCenterX, gridCenterY, CAMERA_MAX_HEIGHT);
        cameraTarget = QVector3D(gridCenterX, gridCenterY, 0.0f);
        const float gridWorldWidth = grid->getWidth() * grid->getCellSize();
        const float gridWorldHeight = grid->getHeight() * grid->getCellSize();
        cameraZoom = std::max(gridWorldWidth, gridWorldHeight) * 0.6f;
    } else { // Orbit mode
        cameraTarget = QVector3D(gridCenterX, 0.0f, gridCenterY);
        cameraYaw = -90.0f;
        cameraPitch = -30.0f;
        cameraZoom = 150.0f;
    }
    updateProjectionMatrix();
    setupCamera();
}

void OpenGLRenderer::setupCamera() {
    viewMatrix.setToIdentity();
    if (cameraMode == CameraMode::TopDown) {
        viewMatrix.lookAt(cameraPosition, cameraTarget, QVector3D(0.0F, 1.0F, 0.0F));
    } else { // Orbit
        QVector3D direction;
        direction.setX(cos(qDegreesToRadians(cameraYaw)) * cos(qDegreesToRadians(cameraPitch)));
        direction.setY(sin(qDegreesToRadians(cameraPitch)));
        direction.setZ(sin(qDegreesToRadians(cameraYaw)) * cos(qDegreesToRadians(cameraPitch)));
        cameraPosition = cameraTarget - direction.normalized() * cameraZoom;

        QVector3D up = QVector3D(0.0f, 1.0f, 0.0f);
        QVector3D right = QVector3D::crossProduct(up, -direction).normalized();
        up = QVector3D::crossProduct(-direction, right).normalized();
        viewMatrix.lookAt(cameraPosition, cameraTarget, up);
    }
    update();
}

void OpenGLRenderer::updateProjectionMatrix() {
    const float aspect = static_cast<float>(width()) / static_cast<float>(height());
    projectionMatrix.setToIdentity();

    if (cameraMode == CameraMode::TopDown) {
        float orthoSize = cameraZoom;
        if (aspect > 1.0f) {
            projectionMatrix.ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, 0.1f, CAMERA_MAX_HEIGHT * 2.0f);
        } else {
            projectionMatrix.ortho(-orthoSize, orthoSize, -orthoSize / aspect, orthoSize / aspect, 0.1f, CAMERA_MAX_HEIGHT * 2.0f);
        }
    } else { // Orbit
        projectionMatrix.perspective(45.0f, aspect, 0.1f, 2000.0f);
    }
}

void OpenGLRenderer::mousePressEvent(QMouseEvent* event) {
    lastMousePos = event->pos();
    isDragging = true;

    if (cameraMode == CameraMode::TopDown && event->button() == Qt::LeftButton) {
        int gridX, gridY;
        if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
            paintTool->startContinuousPainting(grid.get(), gridX, gridY);
            emit cellClicked(gridX, gridY);
        }
    }
    QOpenGLWidget::mousePressEvent(event);
}

void OpenGLRenderer::mouseMoveEvent(QMouseEvent* event) {
    const QPoint delta = event->pos() - lastMousePos;

    if (isDragging) {
        if (cameraMode == CameraMode::Orbit) {
            if (event->buttons() & Qt::LeftButton) {
                panCamera(delta.x(), delta.y());
            } else if (event->buttons() & Qt::RightButton) {
                rotateCamera(delta.x(), delta.y());
            }
        } else { // TopDown
            if (event->buttons() & Qt::LeftButton) { // Painting
                int gridX, gridY;
                if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                    paintTool->updatePaintPosition(gridX, gridY);
                }
            } else if (event->buttons() & Qt::RightButton) { // Panning
                panCamera(delta.x(), delta.y());
            }
        }
    }

    lastMousePos = event->pos();

    int gridX, gridY;
    if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
        if (gridX != hoveredGridX || gridY != hoveredGridY) {
            hoveredGridX = gridX;
            hoveredGridY = gridY;
            const auto* cell = grid->getCell(gridX, gridY);
            if (cell) emit cellHovered(gridX, gridY, *cell);
        }
    } else {
        hoveredGridX = -1;
        hoveredGridY = -1;
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void OpenGLRenderer::mouseReleaseEvent(QMouseEvent* event) {
    isDragging = false;
    if (cameraMode == CameraMode::TopDown && event->button() == Qt::LeftButton) {
        paintTool->stopContinuousPainting();
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void OpenGLRenderer::wheelEvent(QWheelEvent* event) {
    const float delta = event->angleDelta().y();
    float zoomFactor = 1.0f - (delta / 1200.0f);

    if (cameraMode == CameraMode::Orbit && event->modifiers() & Qt::ShiftModifier) {
        moveCameraVertical(delta * 0.1f);
    } else {
        setZoom(cameraZoom * zoomFactor);
    }
    event->accept();
}

void OpenGLRenderer::panCamera(float deltaX, float deltaY) {
    if (cameraMode == CameraMode::TopDown) {
        float sensitivity = cameraZoom * 0.001f;
        cameraPosition.setX(cameraPosition.x() - deltaX * sensitivity);
        cameraPosition.setY(cameraPosition.y() + deltaY * sensitivity);
        cameraTarget.setX(cameraTarget.x() - deltaX * sensitivity);
        cameraTarget.setY(cameraTarget.y() + deltaY * sensitivity);
    } else { // Orbit
        float sensitivity = 0.002f * cameraZoom;
        QVector3D forward = cameraTarget - cameraPosition;
        forward.setY(0);
        forward.normalize();
        QVector3D right = QVector3D::crossProduct(forward, QVector3D(0,1,0));
        cameraTarget -= right * deltaX * sensitivity;
        cameraTarget += forward * deltaY * sensitivity;
    }
    setupCamera();
}

void OpenGLRenderer::rotateCamera(float yawDelta, float pitchDelta) {
    cameraYaw += yawDelta * 0.4f;
    cameraPitch -= pitchDelta * 0.4f;
    cameraPitch = std::max(-89.0f, std::min(89.0f, cameraPitch));
    setupCamera();
}

void OpenGLRenderer::moveCameraVertical(float delta) {
    float sensitivity = 0.005f * cameraZoom;
    cameraTarget.setY(cameraTarget.y() + delta * sensitivity);
    setupCamera();
}

void OpenGLRenderer::setZoom(float zoom) {
    if (cameraMode == CameraMode::TopDown) {
        cameraZoom = std::max(CAMERA_ZOOM_MIN_ORTHO, std::min(CAMERA_ZOOM_MAX_ORTHO, zoom));
    } else {
        cameraZoom = std::max(CAMERA_ZOOM_MIN_PERSP, std::min(CAMERA_ZOOM_MAX_PERSP, zoom));
    }
    updateProjectionMatrix();
    setupCamera();
}

bool OpenGLRenderer::screenToGridCoords(const int screenX, const int screenY, int& gridX, int& gridY) const {
    if (!grid) return false;

    if (cameraMode == CameraMode::TopDown) {
        const float ndcX = (2.0f * screenX) / width() - 1.0f;
        const float ndcY = 1.0f - (2.0f * screenY) / height();
        const QMatrix4x4 invViewProj = (projectionMatrix * viewMatrix).inverted();
        QVector4D worldPos = invViewProj * QVector4D(ndcX, ndcY, 0.0, 1.0);
        if (worldPos.w() == 0.0f) return false;
        worldPos /= worldPos.w();

        const float cellSize = grid->getCellSize();
        gridX = static_cast<int>(qFloor(worldPos.x() / cellSize));
        gridY = static_cast<int>(qFloor(worldPos.y() / cellSize));
    } else { // Orbit - Raycasting on Y=0 plane
        QVector3D nearPoint = QVector3D(screenX, height() - screenY, 0.0f).unproject(viewMatrix, projectionMatrix, QRect(0, 0, width(), height()));
        QVector3D farPoint = QVector3D(screenX, height() - screenY, 1.0f).unproject(viewMatrix, projectionMatrix, QRect(0, 0, width(), height()));
        QVector3D direction = (farPoint - nearPoint).normalized();

        QVector3D planeNormal(0.0f, 1.0f, 0.0f);
        QVector3D planePoint(0.0f, 0.0f, 0.0f);

        float denom = QVector3D::dotProduct(direction, planeNormal);
        if (qAbs(denom) < 1e-6) return false;
        float t = QVector3D::dotProduct(planePoint - nearPoint, planeNormal) / denom;
        if (t < 0) return false;

        QVector3D intersectionPoint = nearPoint + t * direction;
        const float cellSize = grid->getCellSize();
        gridX = static_cast<int>(qFloor(intersectionPoint.x() / cellSize));
        gridY = static_cast<int>(qFloor(intersectionPoint.z() / cellSize));
    }

    return grid->isValidPosition(gridX, gridY);
}

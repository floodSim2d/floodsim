#include "OpenGLRenderer.h"

#include <QWheelEvent>
#include <QtMath>
#include <cmath>

#include "../Simulation/Grid/Grid.h"
#include "../Simulation/Grid/Cell.h"

OpenGLRenderer::OpenGLRenderer(QWidget* parent)
    : QOpenGLWidget(parent),
      grid(nullptr),
      cameraZoom(50.0f),
      cameraTarget(0.0F, 0.0F, 0.0F),
      cameraYaw(-90.0f),
      cameraPitch(-30.0f),
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
    delete paintTool; // Naprawiono memory leak
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

    resetCamera();
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

void OpenGLRenderer::setupCamera() {
    viewMatrix.setToIdentity();

    QVector3D direction;
    direction.setX(cos(qDegreesToRadians(cameraYaw)) * cos(qDegreesToRadians(cameraPitch)));
    direction.setY(sin(qDegreesToRadians(cameraPitch)));
    direction.setZ(sin(qDegreesToRadians(cameraYaw)) * cos(qDegreesToRadians(cameraPitch)));

    cameraPosition = cameraTarget - direction.normalized() * cameraZoom;

    QVector3D up = QVector3D(0.0f, 1.0f, 0.0f);
    QVector3D right = QVector3D::crossProduct(up, -direction).normalized();
    up = QVector3D::crossProduct(-direction, right).normalized();

    viewMatrix.lookAt(cameraPosition, cameraTarget, up);
    update();
}

void OpenGLRenderer::updateProjectionMatrix() {
    const float aspect = static_cast<float>(width()) / static_cast<float>(height());
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(45.0f, aspect, 0.1f, 2000.0f);
}

void OpenGLRenderer::setZoom(const float zoom) {
    cameraZoom = std::max(CAMERA_ZOOM_MIN, std::min(CAMERA_ZOOM_MAX, zoom));
    setupCamera();
}

void OpenGLRenderer::panCamera(float deltaX, float deltaY) {
    QVector3D forward = cameraTarget - cameraPosition;
    forward.setY(0);
    forward.normalize();
    QVector3D right = QVector3D::crossProduct(forward, QVector3D(0,1,0));

    float sensitivity = 0.002f * cameraZoom;

    cameraTarget -= right * deltaX * sensitivity;
    cameraTarget -= forward * deltaY * sensitivity;

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

void OpenGLRenderer::setCameraPanEnabled(bool enabled) {
    cameraPanEnabled = enabled;
    emit cameraPanToggled(enabled);
}

void OpenGLRenderer::resetCamera() {
    if (!grid) return;

    float gridCenterX = grid->getWidth() * grid->getCellSize() * 0.5F;
    float gridCenterZ = grid->getHeight() * grid->getCellSize() * 0.5F;

    cameraTarget = QVector3D(gridCenterX, 0.0f, gridCenterZ);
    cameraYaw = -90.0f;
    cameraPitch = -30.0f;
    cameraZoom = 150.0f;

    setupCamera();
}

void OpenGLRenderer::mousePressEvent(QMouseEvent* event) {
    lastMousePos = event->pos();
    isDragging = true;

    if (event->button() == Qt::LeftButton && !cameraPanEnabled) {
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
        if (cameraPanEnabled) {
            if (event->buttons() & Qt::LeftButton) {
                panCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
            } else if (event->buttons() & Qt::RightButton) {
                rotateCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
            }
        } else if (event->buttons() & Qt::LeftButton) {
            int gridX, gridY;
            if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                paintTool->updatePaintPosition(gridX, gridY);
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
    if (event->button() == Qt::LeftButton && !cameraPanEnabled) {
        paintTool->stopContinuousPainting();
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void OpenGLRenderer::wheelEvent(QWheelEvent* event) {
    const float delta = event->angleDelta().y();

    if (cameraPanEnabled && event->modifiers() & Qt::ShiftModifier) {
        moveCameraVertical(delta * 0.1f);
    } else {
        float zoomFactor = 1.0f - (delta / 1200.0f);
        setZoom(cameraZoom * zoomFactor);
    }
    event->accept();
}

bool OpenGLRenderer::screenToGridCoords(const int screenX, const int screenY, int& gridX, int& gridY) const {
    if (!grid) return false;

    QVector3D nearPoint = QVector3D(screenX, height() - screenY, 0.0f).unproject(viewMatrix, projectionMatrix, QRect(0, 0, width(), height()));
    QVector3D farPoint = QVector3D(screenX, height() - screenY, 1.0f).unproject(viewMatrix, projectionMatrix, QRect(0, 0, width(), height()));
    QVector3D direction = (farPoint - nearPoint).normalized();

    float minT = 0.0f;
    float maxT = 2000.0f;
    const int iterations = 20;

    float intersectionT = -1.0f;

    for (int i = 0; i < iterations; ++i) {
        float midT = (minT + maxT) / 2.0f;
        QVector3D testPoint = nearPoint + direction * midT;

        int testGridX = static_cast<int>(qFloor(testPoint.x() / grid->getCellSize()));
        int testGridY = static_cast<int>(qFloor(testPoint.z() / grid->getCellSize()));

        if (!grid->isValidPosition(testGridX, testGridY)) {
            maxT = midT;
            continue;
        }

        const Cell* cell = grid->getCell(testGridX, testGridY);
        float terrainHeight = cell ? cell->getTerrainHeight() : 0.0f;

        if (testPoint.y() > terrainHeight) {
            minT = midT;
        } else {
            maxT = midT;
            intersectionT = midT;
        }
    }

    if (intersectionT > 0.0f) {
        QVector3D intersectionPoint = nearPoint + direction * intersectionT;
        gridX = static_cast<int>(qFloor(intersectionPoint.x() / grid->getCellSize()));
        gridY = static_cast<int>(qFloor(intersectionPoint.z() / grid->getCellSize()));
        return grid->isValidPosition(gridX, gridY);
    }

    return false;
}

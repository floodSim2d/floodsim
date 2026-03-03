#include "OpenGLRenderer.h"

#include <QWheelEvent>
#include <QtMath>
#include <cmath>

#include "../Utils/Logger.h"
#include "../Simulation/Grid/Grid.h"
#include "../Simulation/Grid/Cell.h"

OpenGLRenderer::OpenGLRenderer(Grid* grid,WaterRenderer* water_renderer ,QWidget* parent)
    : QOpenGLWidget(parent),
      grid(grid),
      cameraMode(CameraMode::TopDown),  // Start with TopDown for compatibility
      cameraZoom(CAMERA_ZOOM_MAX_ORTHO),
      cameraPosition(0.0F, 0.0F, CAMERA_MAX_HEIGHT),
      cameraTarget(0.0F, 0.0F, 0.0F),
      cameraYaw(-90.0f),
      cameraPitch(-30.0f),
      isDragging(false),
      hoveredGridX(-1),
      hoveredGridY(-1),
      paintTool(nullptr),
      cameraPanEnabled(false),
      waterRenderer(water_renderer) {
    setMouseTracking(true);
}

OpenGLRenderer::~OpenGLRenderer() {
    makeCurrent();
    if (waterRenderer) {
        waterRenderer->cleanup();
        delete waterRenderer;
        waterRenderer = nullptr;
    }
    doneCurrent();
}

void OpenGLRenderer::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.15F, 0.15F, 0.15F, 1.0F); // Dark gray background
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    LOG("OpenGL Renderer initialized.");
    LOG(QString("Vendor: %1").arg(reinterpret_cast<const char*>(glGetString(GL_VENDOR))));
    LOG(QString("Renderer: %1").arg(reinterpret_cast<const char*>(glGetString(GL_RENDERER))));
    LOG(QString("Version: %1").arg(reinterpret_cast<const char*>(glGetString(GL_VERSION))));

    if (grid != nullptr) {
        grid->initialize(this);

        waterRenderer->initialize(this);

        // Center camera on grid - position directly above for top-down view
        const float gridCenterX = static_cast<float>(grid->getWidth()) * grid->getCellSize() * 0.5F;
        const float gridCenterY = static_cast<float>(grid->getHeight()) * grid->getCellSize() * 0.5F;
        cameraPosition = QVector3D(gridCenterX, gridCenterY, CAMERA_MAX_HEIGHT);
        cameraTarget = QVector3D(gridCenterX, gridCenterY, 0.0F);
    }

    // Set initial zoom to show entire grid (use the larger dimension)
    const float gridWorldWidth = static_cast<float>(grid->getWidth()) * grid->getCellSize();
    const float gridWorldHeight = static_cast<float>(grid->getHeight()) * grid->getCellSize();
    cameraZoom = std::max(gridWorldWidth, gridWorldHeight) * 0.6F;
    cameraZoom = std::max(CAMERA_ZOOM_MIN_ORTHO, std::min(CAMERA_ZOOM_MAX_ORTHO, cameraZoom));

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

    if (grid && waterRenderer) {
        grid->render(projectionMatrix, viewMatrix);

        waterRenderer->render(projectionMatrix, viewMatrix, grid->getHeightTexture());
    }
}

void OpenGLRenderer:: setCameraMode(CameraMode mode) {
    if (cameraMode == mode) return;
    cameraMode = mode;
    resetCamera();
    emit cameraPanToggled(mode == CameraMode::Orbit);
}

void OpenGLRenderer::setCameraPanEnabled(bool enabled) {
    cameraPanEnabled = enabled;
    setCameraMode(enabled ? CameraMode::Orbit :  CameraMode::TopDown);
}

void OpenGLRenderer:: setupCamera() {
    viewMatrix. setToIdentity();

    if (cameraMode == CameraMode::TopDown) {
        viewMatrix.lookAt(cameraPosition, cameraTarget, QVector3D(0.0F, 1.0F, 0.0F));
    } else { // Orbit mode
        QVector3D direction;
        direction. setX(cos(qDegreesToRadians(cameraYaw)) * cos(qDegreesToRadians(cameraPitch)));
        direction.setY(sin(qDegreesToRadians(cameraPitch)));
        direction.setZ(sin(qDegreesToRadians(cameraYaw)) * cos(qDegreesToRadians(cameraPitch)));
        cameraPosition = cameraTarget - direction. normalized() * cameraZoom;

        QVector3D up = QVector3D(0.0f, 1.0f, 0.0f);
        QVector3D right = QVector3D:: crossProduct(up, -direction).normalized();
        up = QVector3D::crossProduct(-direction, right).normalized();
        viewMatrix.lookAt(cameraPosition, cameraTarget, up);
    }
    update();
}

void OpenGLRenderer::updateProjectionMatrix() {
    const float aspect = static_cast<float>(width()) / static_cast<float>(height());
    projectionMatrix.setToIdentity();

    if (cameraMode == CameraMode::TopDown) {
        const float orthoSize = cameraZoom;
        const float maxDepth = grid ?  grid->getMaxDepth() : DEFAULT_WATER_DEPTH;
        const float nearPlane = 0.1F;
        const float farPlane = CAMERA_MAX_HEIGHT + maxDepth;

        if (aspect > 1.0f) {
            projectionMatrix. ortho(-orthoSize * aspect, orthoSize * aspect,
                                  -orthoSize, orthoSize,
                                  nearPlane, farPlane);
        } else {
            projectionMatrix.ortho(-orthoSize, orthoSize,
                                  -orthoSize / aspect, orthoSize / aspect,
                                  nearPlane, farPlane);
        }
    } else { // Orbit mode - perspective projection
        projectionMatrix.perspective(45.0f, aspect, 0.1f, 2000.0f);
    }
}

void OpenGLRenderer::setZoom(const float zoom) {
    if (cameraMode == CameraMode::TopDown) {
        cameraZoom = std::max(CAMERA_ZOOM_MIN_ORTHO, std::min(CAMERA_ZOOM_MAX_ORTHO, zoom));
    } else {
        cameraZoom = std::max(CAMERA_ZOOM_MIN_PERSP, std::min(CAMERA_ZOOM_MAX_PERSP, zoom));
    }

    updateProjectionMatrix();
    if (cameraMode == CameraMode:: Orbit) {
        setupCamera();  // In Orbit mode, zoom affects camera position
    } else {
        update();
    }
}

void OpenGLRenderer::panCamera(float deltaX, float deltaY) {
    if (cameraMode == CameraMode::TopDown) {
        const float sensitivity = cameraZoom * 0.01f;
        cameraPosition.setX(cameraPosition.x() + deltaX * sensitivity);
        cameraPosition.setY(cameraPosition.y() - deltaY * sensitivity);
        cameraTarget.setX(cameraTarget.x() + deltaX * sensitivity);
        cameraTarget. setY(cameraTarget.y() - deltaY * sensitivity);
    } else { // Orbit mode
        const float sensitivity = 0.002f * cameraZoom;
        QVector3D forward = cameraTarget - cameraPosition;
        forward.setY(0);
        forward.normalize();
        QVector3D right = QVector3D::crossProduct(forward, QVector3D(0, 1, 0));
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

void OpenGLRenderer::resetCamera() {
    if (! grid) {
        return;
    }

    float gridCenterX = grid->getWidth() * grid->getCellSize() * 0.5F;
    float gridCenterY = grid->getHeight() * grid->getCellSize() * 0.5F;

    if (cameraMode == CameraMode::TopDown) {
        cameraPosition = QVector3D(gridCenterX, gridCenterY, CAMERA_MAX_HEIGHT);
        cameraTarget = QVector3D(gridCenterX, gridCenterY, 0.0F);

        // Calculate zoom to show entire grid (use the larger dimension)
        const float gridWorldWidth = grid->getWidth() * grid->getCellSize();
        const float gridWorldHeight = grid->getHeight() * grid->getCellSize();
        cameraZoom = std:: max(gridWorldWidth, gridWorldHeight) * 0.6F;
        cameraZoom = std::max(CAMERA_ZOOM_MIN_ORTHO, std::min(CAMERA_ZOOM_MAX_ORTHO, cameraZoom));
    } else { // Orbit mode
        cameraTarget = QVector3D(gridCenterX, 0.0f, gridCenterY);
        cameraYaw = -90.0f;
        cameraPitch = -30.0f;
        cameraZoom = 150.0f;
    }

    updateProjectionMatrix();
    setupCamera();
}

void OpenGLRenderer::mousePressEvent(QMouseEvent* event) {
    lastMousePos = event->pos();
    isDragging = true;

    if (event->button() == Qt::LeftButton) {
        if (cameraMode == CameraMode::TopDown) {
            int gridX;
            int gridY;
            if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                if (paintTool != nullptr && paintTool->getToolType() != ToolType::Camera) {
                    paintTool->startContinuousPainting(grid, gridX, gridY, false);
                }
                emit cellClicked(gridX, gridY);
            }
        }
    }

    if (event->button() == Qt::RightButton) {
        if (cameraMode == CameraMode::TopDown && !cameraPanEnabled) {
            // right-click for alternate mode painting (only if not in camera pan mode)
            int gridX;
            int gridY;
            if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                if (paintTool != nullptr && paintTool->getToolType() != ToolType::Camera) {
                    paintTool->startContinuousPainting(grid, gridX, gridY, true);
                }
            }
        }
    }

    QOpenGLWidget::mousePressEvent(event);
}

void OpenGLRenderer::mouseMoveEvent(QMouseEvent* event) {
    const QPoint delta = event->pos() - lastMousePos;

    // Handle dragging with LEFT button (LPM)
    if (isDragging && (event->buttons() & Qt::LeftButton) != 0U) {
        if (cameraMode == CameraMode:: Orbit) {
            // W trybie Orbit:  LPM = OBRÓT (jak inspekcja broni w CS)
            rotateCamera(delta.x(), delta.y());
        } else { // TopDown
            if (cameraPanEnabled) {
                // Camera panning mode
                panCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
            } else if (paintTool != nullptr && paintTool->getToolType() != ToolType::Camera) {
                // Paint mode - update current position for continuous painting
                int gridX;
                int gridY;
                if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                    paintTool->updatePaintPosition(gridX, gridY);
                }
            }
        }
        lastMousePos = event->pos();
    }

    // Handle dragging with RIGHT button (PPM)
    if (isDragging && (event->buttons() & Qt::RightButton) != 0U) {
        if (cameraMode == CameraMode::Orbit) {
            panCamera(delta.x(), delta.y());
        } else { // TopDown
            if (cameraPanEnabled) {
                // Camera panning mode
                panCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
            } else if (paintTool != nullptr && paintTool->getToolType() != ToolType::Camera) {
                // Alternate mode painting with right-click
                int gridX;
                int gridY;
                if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                    paintTool->updatePaintPosition(gridX, gridY);
                }
            }
        }
        lastMousePos = event->pos();
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
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        isDragging = false;
        if (cameraMode == CameraMode::TopDown && paintTool != nullptr) {
            paintTool->stopContinuousPainting();
        }
    }

    QOpenGLWidget::mouseReleaseEvent(event);
}

void OpenGLRenderer::wheelEvent(QWheelEvent* event) {
    const float delta = event->angleDelta().y();

    if (cameraMode == CameraMode:: Orbit && (event->modifiers() & Qt::ShiftModifier)) {
        // Shift + scroll in Orbit mode = vertical movement
        moveCameraVertical(delta * 0.1f);
    } else {
        // Normal scroll = zoom
        const float zoomFactor = 1.0f - (delta / 1200.0f);
        setZoom(cameraZoom * zoomFactor);
    }

    event->accept();
}

bool OpenGLRenderer::screenToGridCoords(const int screenX, const int screenY, int& gridX, int& gridY) const {
    if (! grid) {
        return false;
    }

    if (cameraMode == CameraMode::TopDown) {
        // Convert screen coordinates to normalized device coordinates
        const float ndcX = (2.0f * screenX) / width() - 1.0f;
        const float ndcY = 1.0f - (2.0f * screenY) / height();

        // For orthographic projection, unproject directly to world space at z=0 plane
        const QMatrix4x4 invProj = projectionMatrix. inverted();
        const QMatrix4x4 invView = viewMatrix.inverted();

        // Start with NDC position at z=0 (ground plane in view space after projection)
        QVector4D viewPos = invProj * QVector4D(ndcX, ndcY, 0.0F, 1.0F);

        // For orthographic projection, w should be 1, but let's be safe
        if (viewPos.w() != 0.0f) {
            viewPos /= viewPos.w();
        }

        // Now transform from view space to world space
        QVector4D worldPos = invView * viewPos;

        // Convert world coordinates to grid coordinates
        const float cellSize = grid->getCellSize();
        gridX = static_cast<int>(worldPos.x() / cellSize);
        gridY = static_cast<int>(worldPos.y() / cellSize);
    } else { // Orbit mode - Raycasting on Y=0 plane
        QVector3D nearPoint = QVector3D(screenX, height() - screenY, 0.0f).unproject(
            viewMatrix, projectionMatrix, QRect(0, 0, width(), height()));
        QVector3D farPoint = QVector3D(screenX, height() - screenY, 1.0f).unproject(
            viewMatrix, projectionMatrix, QRect(0, 0, width(), height()));
        QVector3D direction = (farPoint - nearPoint).normalized();

        QVector3D planeNormal(0.0f, 1.0f, 0.0f);
        QVector3D planePoint(0.0f, 0.0f, 0.0f);

        float denom = QVector3D:: dotProduct(direction, planeNormal);
        if (qAbs(denom) < 1e-6) return false;

        float t = QVector3D::dotProduct(planePoint - nearPoint, planeNormal) / denom;
        if (t < 0) return false;

        QVector3D intersectionPoint = nearPoint + t * direction;
        const float cellSize = grid->getCellSize();
        gridX = static_cast<int>(qFloor(intersectionPoint.x() / cellSize));
        gridY = static_cast<int>(qFloor(intersectionPoint. z() / cellSize));
    }

    return grid->isValidPosition(gridX, gridY);
}
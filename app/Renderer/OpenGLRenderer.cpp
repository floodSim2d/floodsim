#include "OpenGLRenderer.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

#include "../Simulation/Grid.h"

OpenGLRenderer::OpenGLRenderer(QWidget* parent)
    : QOpenGLWidget(parent),
      grid(nullptr),
      cameraZoom(50.0f),
      cameraPosition(0.0f, 0.0f, 50.0f),
      cameraTarget(0.0f, 0.0f, 0.0f),
      isDragging(false),
      hoveredGridX(-1),
      hoveredGridY(-1) {
    setMouseTracking(true);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
}

OpenGLRenderer::~OpenGLRenderer() {
    makeCurrent();
    grid.reset();
    doneCurrent();
}

void OpenGLRenderer::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    grid = std::make_unique<Grid>(100, 100, 1.0f);
    grid->initialize(this);

    // test heightmap data
    for (int y = 0; y < grid->getHeight(); ++y) {
        for (int x = 0; x < grid->getWidth(); ++x) {
            float fx = static_cast<float>(x) / grid->getWidth();
            float fy = static_cast<float>(y) / grid->getHeight();

            // hills using sine waves
            float height = 5.0f + 3.0f * std::sin(fx * 6.28f) * std::cos(fy * 6.28f);
            height += 1.0f * std::sin(fx * 12.56f + fy * 12.56f);

            grid->setHeightValue(x, y, height);
        }
    }

    grid->updateHeightTexture();

    setupCamera();
}

void OpenGLRenderer::resizeGL(int width, int height) {
    glViewport(0, 0, width, height);

    float aspect = static_cast<float>(width) / static_cast<float>(height);
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(45.0f, aspect, 0.1f, 1000.0f);
}

void OpenGLRenderer::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (grid) {
        grid->render(projectionMatrix, viewMatrix);
    }
}

void OpenGLRenderer::setupCamera() {
    viewMatrix.setToIdentity();
    viewMatrix.lookAt(cameraPosition, cameraTarget, QVector3D(0.0f, 1.0f, 0.0f));
}

void OpenGLRenderer::setZoom(float zoom) {
    cameraZoom = std::max(10.0f, std::min(200.0f, zoom));

    // Update camera position based on zoom
    float gridCenterX = grid->getWidth() * grid->getCellSize() * 0.5f;
    float gridCenterY = grid->getHeight() * grid->getCellSize() * 0.5f;

    cameraPosition = QVector3D(gridCenterX, gridCenterY, cameraZoom);
    cameraTarget = QVector3D(gridCenterX, gridCenterY, 0.0f);

    setupCamera();
    update();
}

void OpenGLRenderer::panCamera(float deltaX, float deltaY) {
    float sensitivity = cameraZoom * 0.01f;

    cameraPosition.setX(cameraPosition.x() + deltaX * sensitivity);
    cameraPosition.setY(cameraPosition.y() - deltaY * sensitivity);
    cameraTarget.setX(cameraTarget.x() + deltaX * sensitivity);
    cameraTarget.setY(cameraTarget.y() - deltaY * sensitivity);

    setupCamera();
    update();
}

void OpenGLRenderer::resetCamera() {
    cameraZoom = 50.0f;

    float gridCenterX = grid->getWidth() * grid->getCellSize() * 0.5f;
    float gridCenterY = grid->getHeight() * grid->getCellSize() * 0.5f;

    cameraPosition = QVector3D(gridCenterX, gridCenterY, cameraZoom);
    cameraTarget = QVector3D(gridCenterX, gridCenterY, 0.0f);

    setupCamera();
    update();
}

void OpenGLRenderer::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
        isDragging = true;

        int gridX, gridY;
        if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
            emit cellClicked(gridX, gridY);
        }
    }

    QOpenGLWidget::mousePressEvent(event);
}

void OpenGLRenderer::mouseMoveEvent(QMouseEvent* event) {
    // Handle camera panning
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->pos() - lastMousePos;
        panCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
        lastMousePos = event->pos();
    }

    // Handle hover - show height value
    int gridX, gridY;
    if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
        if (gridX != hoveredGridX || gridY != hoveredGridY) {
            hoveredGridX = gridX;
            hoveredGridY = gridY;

            float height = grid->getHeightValue(gridX, gridY);
            emit heightValueHovered(gridX, gridY, height);
        }
    } else {
        hoveredGridX = -1;
        hoveredGridY = -1;
    }

    QOpenGLWidget::mouseMoveEvent(event);
}

void OpenGLRenderer::wheelEvent(QWheelEvent* event) {
    float delta = event->angleDelta().y() / 120.0f;
    float zoomFactor = 1.0f + delta * 0.1f;
    setZoom(cameraZoom / zoomFactor);

    event->accept();
}

bool OpenGLRenderer::screenToGridCoords(int screenX, int screenY, int& gridX, int& gridY) const {
    if (!grid) return false;

    // Convert screen coordinates to OpenGL normalized device coordinates
    float x = (2.0f * screenX) / width() - 1.0f;
    float y = 1.0f - (2.0f * screenY) / height();

    // Create matrices for unprojection
    QMatrix4x4 invView = viewMatrix.inverted();
    QMatrix4x4 invProj = projectionMatrix.inverted();

    // Transform to view space
    QVector4D rayClip(x, y, -1.0f, 1.0f);
    QVector4D rayEye = invProj * rayClip;
    rayEye = QVector4D(rayEye.x(), rayEye.y(), -1.0f, 0.0f);

    // Transform to world space
    QVector4D rayWorldTemp = invView * rayEye;
    QVector3D rayWorld(rayWorldTemp.x(), rayWorldTemp.y(), rayWorldTemp.z());
    rayWorld.normalize();

    // Ray-plane intersection (ground plane at z=0)
    QVector3D rayOrigin = cameraPosition;
    float t = -rayOrigin.z() / rayWorld.z();

    if (t < 0) return false;  // Ray pointing away from ground

    QVector3D intersectionPoint = rayOrigin + rayWorld * t;

    // Convert world coordinates to grid coordinates
    float cellSize = grid->getCellSize();
    gridX = static_cast<int>(intersectionPoint.x() / cellSize);
    gridY = static_cast<int>(intersectionPoint.y() / cellSize);

    return grid->isValidPosition(gridX, gridY);
}

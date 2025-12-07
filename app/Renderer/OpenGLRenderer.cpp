#include "OpenGLRenderer.h"

#include <QWheelEvent>
#include <cmath>

#include "../Simulation/Grid/Grid.h"

OpenGLRenderer::OpenGLRenderer(QWidget* parent)
    : QOpenGLWidget(parent),
      grid(nullptr),
      cameraZoom(200.0f),
      cameraPosition(0.0f, 0.0f, 100.0f),
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

    glClearColor(0.15F, 0.15F, 0.15F, 1.0F);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    qDebug() << "OpenGL Renderer initialized.";
    qDebug() << "Vendor:" << reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    qDebug() << "Renderer:" << reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    qDebug() << "Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));

    grid = std::make_unique<Grid>(0, 0, 1.0F);
    grid->initialize(this);

    // Center camera on grid
    const float gridCenterX = static_cast<float>(grid->getWidth()) * grid->getCellSize() * 0.5F;
    const float gridCenterY = static_cast<float>(grid->getHeight()) * grid->getCellSize() * 0.5F;
    cameraPosition = QVector3D(gridCenterX, gridCenterY, 100.0F);
    cameraTarget = QVector3D(gridCenterX, gridCenterY, 0.0F);

    // Set initial zoom to show entire grid (use the larger dimension)
    const float gridWorldWidth = static_cast<float>(grid->getWidth()) * grid->getCellSize();
    const float gridWorldHeight = static_cast<float>(grid->getHeight()) * grid->getCellSize();
    cameraZoom = std::max(gridWorldWidth, gridWorldHeight) * 0.6F;
    cameraZoom = std::max(10.0F, std::min(200.0F, cameraZoom));

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
    viewMatrix.lookAt(cameraPosition, cameraTarget, QVector3D(0.0f, 1.0f, 0.0f));
}

void OpenGLRenderer::updateProjectionMatrix() {
    const float aspect = static_cast<float>(width()) / static_cast<float>(height());
    projectionMatrix.setToIdentity();
    const float orthoSize = cameraZoom;
    if (aspect > 1.0f) {
        projectionMatrix.ortho(-orthoSize * aspect, orthoSize * aspect,
                              -orthoSize, orthoSize,
                              0.1f, 1000.0f);
    } else {
        projectionMatrix.ortho(-orthoSize, orthoSize,
                              -orthoSize / aspect, orthoSize / aspect,
                              0.1f, 1000.0f);
    }
}


void OpenGLRenderer::setZoom(const float zoom) {
    cameraZoom = std::max(10.0f, std::min(200.0f, zoom));

    updateProjectionMatrix();
    update();
}

void OpenGLRenderer::panCamera(float deltaX, float deltaY) {
    const float sensitivity = cameraZoom * 0.01f;

    cameraPosition.setX(cameraPosition.x() + deltaX * sensitivity);
    cameraPosition.setY(cameraPosition.y() - deltaY * sensitivity);
    cameraTarget.setX(cameraTarget.x() + deltaX * sensitivity);
    cameraTarget.setY(cameraTarget.y() - deltaY * sensitivity);

    setupCamera();
    update();
}

void OpenGLRenderer::resetCamera() {
    if (!grid) {
        return;
    }

    float gridCenterX = grid->getWidth() * grid->getCellSize() * 0.5f;
    float gridCenterY = grid->getHeight() * grid->getCellSize() * 0.5f;

    cameraPosition = QVector3D(gridCenterX, gridCenterY, 100.0f);
    cameraTarget = QVector3D(gridCenterX, gridCenterY, 0.0f);

    // Calculate zoom to show entire grid (use the larger dimension)
    const float gridWorldWidth = grid->getWidth() * grid->getCellSize();
    const float gridWorldHeight = grid->getHeight() * grid->getCellSize();
    cameraZoom = std::max(gridWorldWidth, gridWorldHeight) * 0.6f;
    // Clamp to valid zoom range
    cameraZoom = std::max(10.0f, std::min(200.0f, cameraZoom));

    updateProjectionMatrix();
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
    // Handle camera moving while mouse dragging
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        const QPoint delta = event->pos() - lastMousePos;
        panCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
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

void OpenGLRenderer::wheelEvent(QWheelEvent* event) {
    const float delta = event->angleDelta().y() / 120.0F;
    const float zoomFactor = 1.0F + (delta * 0.1F);
    setZoom(cameraZoom / zoomFactor);

    event->accept();
}

bool OpenGLRenderer::screenToGridCoords(const int screenX, const int screenY, int& gridX, int& gridY) const {
    if (!grid) { return
        false;
    }

    // Convert screen coordinates to normalized device coordinates
    const float ndcX = (2.0f * screenX) / width() - 1.0f;
    const float ndcY = 1.0f - (2.0f * screenY) / height();

    // For orthographic projection, unproject directly to world space at z=0 plane
    // The matrices are applied in order: projection * view * modelPos
    // So to reverse: invView * invProj * ndcPos
    const QMatrix4x4 invProj = projectionMatrix.inverted();
    const QMatrix4x4 invView = viewMatrix.inverted();

    // Start with NDC position at z=0 (ground plane in view space after projection)
    QVector4D viewPos = invProj * QVector4D(ndcX, ndcY, 0.0F, 1.0F);

    // For orthographic projection, w should be 1, but let's be safe
    if (viewPos.w() != 0.0f) {
        viewPos /= viewPos.w();
    }

    // Now transform from view space to world space
    // We want the point at z=0 in world space, so we need to find where the ray intersects z=0
    // For orthographic, the ray direction is always along the view direction
    QVector4D worldPos = invView * viewPos;

    // Convert world coordinates to grid coordinates
    const float cellSize = grid->getCellSize();
    gridX = static_cast<int>(worldPos.x() / cellSize);
    gridY = static_cast<int>(worldPos.y() / cellSize);

    return grid->isValidPosition(gridX, gridY);
}

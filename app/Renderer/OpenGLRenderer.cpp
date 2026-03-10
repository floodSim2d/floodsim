#include "OpenGLRenderer.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>
#include <cmath>

#include "../Utils/Logger.h"
#include "../Simulation/Grid/Grid.h"
#include "../Simulation/Grid/Cell.h"

OpenGLRenderer::OpenGLRenderer(Grid* grid, WaterRenderer* water_renderer, QWidget* parent)
    : QOpenGLWidget(parent),
      grid(grid),
      waterRenderer(water_renderer),
      cameraMode(CameraMode::TopDown),
      cameraZoom(CAMERA_ZOOM_MAX_ORTHO),
      cameraPosition(0.0F, 0.0F, CAMERA_MAX_HEIGHT),
      cameraTarget(0.0F, 0.0F, 0.0F),
      cameraYaw(-90.0f),
      cameraPitch(-45.0f),
      isDragging(false),
      hoveredGridX(-1),
      hoveredGridY(-1),
      orbitFocusPoint(0.0f, 0.0f, 0.0f),
      panAnchorWorld(0.0f, 0.0f, 0.0f),
      isPanning(false),
      isOrbiting(false),
      paintTool(nullptr),
      cameraPanEnabled(false) {
    setMouseTracking(true);
}

OpenGLRenderer::~OpenGLRenderer() = default;

// ============================================================================
// OpenGL lifecycle
// ============================================================================

void OpenGLRenderer::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.15F, 0.15F, 0.15F, 1.0F);
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

        const float gridCenterX = static_cast<float>(grid->getWidth())  * grid->getCellSize() * 0.5F;
        const float gridCenterY = static_cast<float>(grid->getHeight()) * grid->getCellSize() * 0.5F;
        cameraPosition = QVector3D(gridCenterX, gridCenterY, CAMERA_MAX_HEIGHT);
        cameraTarget   = QVector3D(gridCenterX, gridCenterY, 0.0F);
        orbitFocusPoint = cameraTarget;
    }

    const float gridWorldWidth  = static_cast<float>(grid->getWidth())  * grid->getCellSize();
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

    if (grid != nullptr && waterRenderer != nullptr) {
        grid->render(projectionMatrix, viewMatrix, cameraPosition);
        waterRenderer->render(projectionMatrix, viewMatrix, grid->getHeightTexture(), cameraPosition);
    }
}

// ============================================================================
// Camera setup & projection
// ============================================================================

void OpenGLRenderer::setupCamera() {
    viewMatrix.setToIdentity();

    if (cameraMode == CameraMode::TopDown) {
        viewMatrix.lookAt(cameraPosition, cameraTarget, QVector3D(0.0F, 1.0F, 0.0F));
    } else {
        // ----------------------------------------------------------------
        // Orbit mode — Z-up coordinate system.
        // pitch is NEGATIVE (range -89° to -5°), meaning the camera looks
        // downward onto the terrain.
        // We negate sin(pitch) so that offset.z is always POSITIVE,
        // keeping the camera above the ground plane at all times.
        //   pitch = -89° → offset.z ≈ +1.00  (nearly top-down)
        //   pitch = -45° → offset.z ≈ +0.71  (classic 45° view)
        //   pitch =  -5° → offset.z ≈ +0.09  (nearly at horizon)
        // ----------------------------------------------------------------
        const float yawRad   = qDegreesToRadians(cameraYaw);
        const float pitchRad = qDegreesToRadians(cameraPitch);

        const float cosPitch = std::cos(pitchRad);
        QVector3D offset(
            std::cos(yawRad) * cosPitch,
            std::sin(yawRad) * cosPitch,
            -std::sin(pitchRad)   // NEGATED: keeps camera above ground for negative pitch
        );
        cameraPosition = cameraTarget + offset.normalized() * cameraZoom;

        viewMatrix.lookAt(cameraPosition, cameraTarget, QVector3D(0.0F, 0.0F, 1.0F));
    }

    update();
}

void OpenGLRenderer::updateProjectionMatrix() {
    const float aspect = static_cast<float>(width()) / static_cast<float>(height());
    projectionMatrix.setToIdentity();

    if (cameraMode == CameraMode::TopDown) {
        const float orthoSize = cameraZoom;
        const float maxDepth  = grid ? grid->getMaxDepth() : DEFAULT_WATER_DEPTH;
        const float nearPlane = 0.1F;
        const float farPlane  = CAMERA_MAX_HEIGHT + maxDepth;

        if (aspect > 1.0f) {
            projectionMatrix.ortho(-orthoSize * aspect, orthoSize * aspect,
                                   -orthoSize, orthoSize,
                                   nearPlane, farPlane);
        } else {
            projectionMatrix.ortho(-orthoSize, orthoSize,
                                   -orthoSize / aspect, orthoSize / aspect,
                                   nearPlane, farPlane);
        }
    } else {
        projectionMatrix.perspective(45.0f, aspect, 0.5f, 5000.0f);
    }
}

// ============================================================================
// Public camera API
// ============================================================================

void OpenGLRenderer::setCameraMode(CameraMode mode) {
    if (cameraMode == mode) return;
    cameraMode = mode;
    resetCamera();
    emit cameraPanToggled(mode == CameraMode::Orbit);
}

void OpenGLRenderer::setCameraPanEnabled(bool enabled) {
    cameraPanEnabled = enabled;
    setCameraMode(enabled ? CameraMode::Orbit : CameraMode::TopDown);
}

void OpenGLRenderer::setZoom(const float zoom) {
    if (cameraMode == CameraMode::TopDown) {
        cameraZoom = std::max(CAMERA_ZOOM_MIN_ORTHO, std::min(CAMERA_ZOOM_MAX_ORTHO, zoom));
    } else {
        cameraZoom = std::max(CAMERA_ZOOM_MIN_PERSP, std::min(CAMERA_ZOOM_MAX_PERSP, zoom));
    }

    updateProjectionMatrix();
    if (cameraMode == CameraMode::Orbit) {
        setupCamera();
    } else {
        update();
    }
}

void OpenGLRenderer::panCamera(float deltaX, float deltaY) {
    if (cameraMode == CameraMode::TopDown) {
        const float sensitivity = cameraZoom * 0.01f;
        // In Qt, mouse Y increases downward. Moving mouse down (deltaY > 0) should
        // pan the map downward (camera moves up in world Y → target.y decreases).
        cameraPosition.setX(cameraPosition.x() - deltaX * sensitivity);
        cameraPosition.setY(cameraPosition.y() + deltaY * sensitivity);
        cameraTarget.setX(cameraTarget.x() - deltaX * sensitivity);
        cameraTarget.setY(cameraTarget.y() + deltaY * sensitivity);
    } else {
        // Fallback (used when no anchor is available)
        const float sensitivity = 0.002f * cameraZoom;
        const float yawRad = qDegreesToRadians(cameraYaw);
        // Right vector in XY plane
        QVector3D right(std::sin(yawRad), -std::cos(yawRad), 0.0f);
        // Forward vector in XY plane (projected)
        QVector3D forward(-std::cos(yawRad), -std::sin(yawRad), 0.0f);
        cameraTarget -= right   * deltaX * sensitivity;
        cameraTarget += forward * deltaY * sensitivity;
    }
    setupCamera();
}

void OpenGLRenderer::rotateCamera(float yawDelta, float pitchDelta) {
    cameraYaw   += yawDelta   * 0.4f;
    cameraPitch += pitchDelta * 0.4f;
    // Clamp pitch: PITCH_MAX is -5° (nearly horizontal), PITCH_MIN is -89° (top-down)
    cameraPitch = std::max(CAMERA_PITCH_MIN, std::min(CAMERA_PITCH_MAX, cameraPitch));
    setupCamera();
}

void OpenGLRenderer::moveCameraVertical(float delta) {
    const float sensitivity = 0.005f * cameraZoom;
    cameraTarget.setZ(cameraTarget.z() + delta * sensitivity);
    setupCamera();
}

void OpenGLRenderer::resetCamera() {
    if (!grid) return;

    const float gridCenterX = grid->getWidth()  * grid->getCellSize() * 0.5F;
    const float gridCenterY = grid->getHeight() * grid->getCellSize() * 0.5F;

    if (cameraMode == CameraMode::TopDown) {
        cameraPosition  = QVector3D(gridCenterX, gridCenterY, CAMERA_MAX_HEIGHT);
        cameraTarget    = QVector3D(gridCenterX, gridCenterY, 0.0F);
        orbitFocusPoint = cameraTarget;

        const float gridWorldWidth  = grid->getWidth()  * grid->getCellSize();
        const float gridWorldHeight = grid->getHeight() * grid->getCellSize();
        cameraZoom = std::max(gridWorldWidth, gridWorldHeight) * 0.6F;
        cameraZoom = std::max(CAMERA_ZOOM_MIN_ORTHO, std::min(CAMERA_ZOOM_MAX_ORTHO, cameraZoom));
    } else {
        cameraTarget    = QVector3D(gridCenterX, gridCenterY, 0.0F);
        orbitFocusPoint = cameraTarget;
        cameraYaw   = -90.0f;   // looking from south
        cameraPitch = -45.0f;   // 45° tilt downward
        cameraZoom  = std::max(grid->getWidth(), grid->getHeight()) * grid->getCellSize() * 0.8f;
        cameraZoom  = std::max(CAMERA_ZOOM_MIN_PERSP, std::min(CAMERA_ZOOM_MAX_PERSP, cameraZoom));
    }

    updateProjectionMatrix();
    setupCamera();
}

// ============================================================================
// Google-Maps-style raycasting helper
// Casts a ray from the given screen pixel onto the world ground plane (Z = 0).
// Works for both orthographic (TopDown) and perspective (Orbit) projections.
// ============================================================================
bool OpenGLRenderer::screenToGroundPlane(int screenX, int screenY, QVector3D& worldPos) const {
    // Qt's QOpenGLWidget has Y increasing downward; unproject expects Y flipped.
    const int flippedY = height() - screenY - 1;
    const QRect viewport(0, 0, width(), height());

    const QVector3D nearPt = QVector3D(screenX, flippedY, 0.0f)
                                .unproject(viewMatrix, projectionMatrix, viewport);
    const QVector3D farPt  = QVector3D(screenX, flippedY, 1.0f)
                                .unproject(viewMatrix, projectionMatrix, viewport);

    QVector3D dir = (farPt - nearPt).normalized();

    // Intersect with the horizontal ground plane: Z = 0
    // nearPt + t * dir = (x, y, 0) → t = -nearPt.z() / dir.z()
    if (qAbs(dir.z()) < 1e-6f) return false;   // ray is parallel to ground

    const float t = -nearPt.z() / dir.z();
    if (t < 0.0f) return false;                 // intersection behind camera

    worldPos = nearPt + t * dir;
    return true;
}

// ============================================================================
// Mouse events
// ============================================================================

void OpenGLRenderer::mousePressEvent(QMouseEvent* event) {
    lastMousePos = event->pos();

    if (cameraMode == CameraMode::Orbit) {
        if (event->button() == Qt::LeftButton) {
            // --- ORBIT: remember the ground point under the cursor ---
            // Do NOT call setupCamera() here — that would snap the camera
            // before the user moves the mouse, causing a visible jump.
            isOrbiting = true;
            QVector3D groundPt;
            if (screenToGroundPlane(event->pos().x(), event->pos().y(), groundPt)) {
                orbitFocusPoint = groundPt;
                // cameraTarget stays where it is; we only set the orbit pivot.
                // It will be updated to orbitFocusPoint on the first move event.
            }
        } else if (event->button() == Qt::RightButton) {
            // --- PAN: anchor the ground point under the cursor ---
            isPanning = true;
            QVector3D groundPt;
            if (screenToGroundPlane(event->pos().x(), event->pos().y(), groundPt)) {
                panAnchorWorld = groundPt;
            }
        }
    } else { // TopDown
        if (event->button() == Qt::LeftButton) {
            isDragging = true;
            int gridX, gridY;
            if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                if (paintTool != nullptr && paintTool->getToolType() != ToolType::Camera) {
                    paintTool->startContinuousPainting(grid, gridX, gridY, false);
                }
                emit cellClicked(gridX, gridY);
            }
        } else if (event->button() == Qt::RightButton && !cameraPanEnabled) {
            isDragging = true;
            int gridX, gridY;
            if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                if (paintTool != nullptr && paintTool->getToolType() != ToolType::Camera) {
                    paintTool->startContinuousPainting(grid, gridX, gridY, true);
                }
            }
        } else if (event->button() == Qt::RightButton && cameraPanEnabled) {
            isDragging = true;
        }
    }

    QOpenGLWidget::mousePressEvent(event);
}

void OpenGLRenderer::mouseMoveEvent(QMouseEvent* event) {
    const QPoint delta = event->pos() - lastMousePos;

    if (cameraMode == CameraMode::Orbit) {
        // ------------------------------------------------------------------
        // LEFT BUTTON — ORBIT around the focus point picked at press time
        // ------------------------------------------------------------------
        if ((event->buttons() & Qt::LeftButton) && isOrbiting) {
            // Rotate yaw & pitch
            cameraYaw   += static_cast<float>(delta.x()) * 0.4f;
            cameraPitch -= static_cast<float>(delta.y()) * 0.4f;
            cameraPitch  = std::max(CAMERA_PITCH_MIN, std::min(CAMERA_PITCH_MAX, cameraPitch));

            // On the first move event the target snaps to the orbit focus point.
            // Subsequent moves keep rotating around that same point.
            cameraTarget = orbitFocusPoint;
            setupCamera();
            lastMousePos = event->pos();
        }

        // ------------------------------------------------------------------
        // RIGHT BUTTON — GRAB-AND-DRAG (pan)
        // The world point that was under the cursor at press-time must stay
        // under the cursor at all times → move cameraTarget so it does.
        // ------------------------------------------------------------------
        if ((event->buttons() & Qt::RightButton) && isPanning) {
            QVector3D currentGroundPt;
            if (screenToGroundPlane(event->pos().x(), event->pos().y(), currentGroundPt)) {
                // Shift target so the anchor point returns under the cursor
                const QVector3D shift = panAnchorWorld - currentGroundPt;
                cameraTarget    += shift;
                orbitFocusPoint += shift;   // keep orbit focus in sync
                setupCamera();
                // anchor stays fixed in world space — no update needed
            }
            lastMousePos = event->pos();
        }
    } else { // TopDown
        if (isDragging && (event->buttons() & Qt::LeftButton)) {
            if (cameraPanEnabled) {
                panCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
            } else if (paintTool != nullptr && paintTool->getToolType() != ToolType::Camera) {
                int gridX, gridY;
                if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                    paintTool->updatePaintPosition(gridX, gridY);
                }
            }
            lastMousePos = event->pos();
        }

        if (isDragging && (event->buttons() & Qt::RightButton)) {
            if (cameraPanEnabled) {
                panCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
            } else if (paintTool != nullptr && paintTool->getToolType() != ToolType::Camera) {
                int gridX, gridY;
                if (screenToGridCoords(event->pos().x(), event->pos().y(), gridX, gridY)) {
                    paintTool->updatePaintPosition(gridX, gridY);
                }
            }
            lastMousePos = event->pos();
        }
    }

    // Hover highlight (works in both modes)
    int gridX, gridY;
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
    // Each button independently clears its own state flag.
    // This prevents releasing one button from killing the other button's drag.
    if (event->button() == Qt::LeftButton) {
        isOrbiting = false;
        if (cameraMode == CameraMode::TopDown) {
            isDragging = false;
            if (paintTool != nullptr) {
                paintTool->stopContinuousPainting();
            }
        }
    }
    if (event->button() == Qt::RightButton) {
        isPanning = false;
        if (cameraMode == CameraMode::TopDown) {
            isDragging = false;
            if (paintTool != nullptr) {
                paintTool->stopContinuousPainting();
            }
        }
    }

    QOpenGLWidget::mouseReleaseEvent(event);
}

// ============================================================================
// Scroll — ZOOM TO CURSOR (Google Maps style)
// Algorithm:
//   1. Find the world point currently under the cursor (groundPt).
//   2. Scale the camera distance (cameraZoom) by the zoom factor.
//   3. Move cameraTarget toward the cursor point so that groundPt stays
//      at the same screen position after the zoom.
//
// Math:  before zoom: screenPos(cursorPt) == cursor
//        after zoom:  camera moved; to keep cursorPt on screen we need
//                     cameraTarget' such that cursorPt maps to cursor again.
//        Since the view is a perspective camera at distance R looking at T:
//            cursorPt stays fixed ↔ T' = cursorPt + (T - cursorPt) * (R'/R)
// ============================================================================
void OpenGLRenderer::wheelEvent(QWheelEvent* event) {
    // Prefer pixelDelta for smooth macOS trackpad scrolling;
    // fall back to angleDelta (mouse wheel gives 120 per notch).
    float scrollAmount = 0.0f;
    if (!event->pixelDelta().isNull()) {
        // pixelDelta is in physical pixels — scale to roughly match angleDelta behaviour
        scrollAmount = static_cast<float>(event->pixelDelta().y()) * 4.0f;
    } else {
        scrollAmount = static_cast<float>(event->angleDelta().y());
    }

    if (cameraMode == CameraMode::Orbit) {
        if ((event->modifiers() & Qt::ShiftModifier) != 0) {
            // Shift+scroll → vertical pan (raise/lower the look-at point)
            moveCameraVertical(scrollAmount * 0.1f);
        } else {
            // ---- ZOOM TO CURSOR ----
            // scrollAmount > 0 → scroll up → zoom IN (reduce distance)
            // zoomFactor < 1 zooms in, > 1 zooms out
            const float zoomFactor = 1.0f - (scrollAmount / 1200.0f);

            QVector3D cursorGroundPt;
            const bool hasCursorPt = screenToGroundPlane(
                static_cast<int>(event->position().x()),
                static_cast<int>(event->position().y()),
                cursorGroundPt);

            const float oldZoom = cameraZoom;
            const float newZoom = std::max(CAMERA_ZOOM_MIN_PERSP,
                                  std::min(CAMERA_ZOOM_MAX_PERSP, cameraZoom * zoomFactor));
            cameraZoom = newZoom;

            if (hasCursorPt && oldZoom > 0.0f) {
                // Keep cursorGroundPt fixed on screen:
                //   T' = cursorPt + (T - cursorPt) * (newZoom / oldZoom)
                // When zooming IN (newZoom < oldZoom), ratio < 1 → target
                // moves toward the cursor point. ✓
                const float ratio = newZoom / oldZoom;
                cameraTarget    = cursorGroundPt + (cameraTarget    - cursorGroundPt) * ratio;
                orbitFocusPoint = cursorGroundPt + (orbitFocusPoint - cursorGroundPt) * ratio;
            }

            updateProjectionMatrix();
            setupCamera();
        }
    } else { // TopDown
        const float zoomFactor = 1.0f - (scrollAmount / 1200.0f);
        setZoom(cameraZoom * zoomFactor);
    }

    event->accept();
}

// ============================================================================
// Screen → Grid coordinate mapping
// ============================================================================
bool OpenGLRenderer::screenToGridCoords(const int screenX, const int screenY,
                                         int& gridX, int& gridY) const {
    if (!grid) return false;

    if (cameraMode == CameraMode::TopDown) {
        // Orthographic: unproject directly to world XY plane
        const float ndcX = (2.0f * screenX) / width()  - 1.0f;
        const float ndcY = 1.0f - (2.0f * screenY) / height();

        const QMatrix4x4 invProj = projectionMatrix.inverted();
        const QMatrix4x4 invView = viewMatrix.inverted();

        QVector4D viewPos = invProj * QVector4D(ndcX, ndcY, 0.0F, 1.0F);
        if (viewPos.w() != 0.0f) viewPos /= viewPos.w();

        QVector4D worldPos = invView * viewPos;

        const float cellSize = grid->getCellSize();
        gridX = static_cast<int>(worldPos.x() / cellSize);
        gridY = static_cast<int>(worldPos.y() / cellSize);
    } else {
        // Orbit: raycast onto Z=0 ground plane (XY terrain system)
        QVector3D groundPt;
        if (!screenToGroundPlane(screenX, screenY, groundPt)) return false;

        const float cellSize = grid->getCellSize();
        gridX = static_cast<int>(qFloor(groundPt.x() / cellSize));
        gridY = static_cast<int>(qFloor(groundPt.y() / cellSize));
    }

    return grid->isValidPosition(gridX, gridY);
}
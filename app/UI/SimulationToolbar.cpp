#include "SimulationToolbar.h"

#include <QAction>
#include <QInputDialog>
#include <QLabel>

#include "../Simulation/Grid/Grid.h"
#include "../Simulation/Grid/Cell.h"
#include "../Simulation/FlowModel/FlowModel.h"
#include "../Renderer/OpenGLRenderer.h"
#include "../WorldConstants.h"

SimulationToolbar::SimulationToolbar(Grid* grid, FlowModel* flowModel, OpenGLRenderer* renderer, QWidget* parent)
    : QToolBar(parent),
      grid(grid),
      flowModel(flowModel),
      renderer(renderer),
      cellInfoLabel(nullptr),
      viewToggleAction(nullptr)
{
    setWindowTitle("Toolbar");
    setupActions();
    setupCellInfoDisplay();
}

void SimulationToolbar::setupActions() {
    QAction* playAction  = addAction("▶ Start");
    QAction* pauseAction = addAction("⏸ Stop");
    QAction* stepAction  = addAction("⏭ Krok");
    addSeparator();
    QAction* resetAction = addAction("Reset");

    connect(playAction, &QAction::triggered, this, [this]() {
        flowModel->play();
    });

    connect(pauseAction, &QAction::triggered, this, [this]() {
        flowModel->pause();
    });

    connect(stepAction, &QAction::triggered, this, [this]() {
        flowModel->step();
        emit statusMessageRequested("Krok symulacji wykonany");
    });

    connect(resetAction, &QAction::triggered, this, [this]() {
        flowModel->stop();
        grid->clearHeightmap();
        renderer->makeCurrent();
        grid->updateHeightTexture();
        grid->updateHeightTexture();
        renderer->doneCurrent();
        renderer->update();
        emit statusMessageRequested("Symulacja zresetowana");
    });

    // ---- 2D / 3D toggle ----
    addSeparator();
    viewToggleAction = addAction("🌐 3D");
    viewToggleAction->setCheckable(true);
    viewToggleAction->setChecked(false);  // starts in 2D (TopDown)
    viewToggleAction->setToolTip(
        "Przełącz między widokiem 2D (z góry, edycja) a 3D (orbit, nawigacja)\n"
        "Skrót: klawisz Tab"
    );

    connect(viewToggleAction, &QAction::toggled, this, [this](bool is3D) {
        renderer->setCameraMode(is3D ? CameraMode::Orbit : CameraMode::TopDown);
        viewToggleAction->setText(is3D ? "🗺 2D" : "🌐 3D");
        emit statusMessageRequested(is3D
            ? "Widok 3D — LPM: obracaj | PPM: przesuń | Scroll: zoom"
            : "Widok 2D — rysuj teren narzędziami z lewego panelu");
    });

    // Keep the button in sync when camera mode changes from outside (e.g. ToolPanel)
    connect(renderer, &OpenGLRenderer::cameraPanToggled, this, [this](bool is3D) {
        // Block signals to avoid re-entrancy loop
        QSignalBlocker blocker(viewToggleAction);
        viewToggleAction->setChecked(is3D);
        viewToggleAction->setText(is3D ? "🗺 2D" : "🌐 3D");
    });
}

void SimulationToolbar::setupCellInfoDisplay() {
    addSeparator();

    cellInfoLabel = new QLabel("Wysokość: ---", this);
    cellInfoLabel->setStyleSheet(
        "QLabel { padding: 5px; background-color: rgba(0, 0, 0, 0.1); border-radius: 3px; }"
    );
    cellInfoLabel->setMinimumWidth(200);
    addWidget(cellInfoLabel);

    connect(renderer, &OpenGLRenderer::cellHovered, this, &SimulationToolbar::updateCellInfo);
}

void SimulationToolbar::updateCellInfo(int gridX, int gridY, const Cell& cell) {
    const float cs = grid->getCellSize();
    const float worldX = World::toDisplay(gridX * cs);
    const float worldY = World::toDisplay(gridY * cs);

    QString label = QString("Wysokość terenu: %1 %2")
        .arg(World::toDisplay(cell.getTerrainHeight()), 0, 'f', 1)
        .arg(World::UNIT_LABEL);

    if (const auto waterDepth = cell.getWaterDepth(); waterDepth > 0.0F) {
        label += QString(" | Głębokość wody: %1 %2")
            .arg(World::toDisplay(waterDepth), 0, 'f', 1).arg(World::UNIT_LABEL);
        label += QString(" | Całkowita wys.: %1 %2")
            .arg(World::toDisplay(cell.getTotalHeight()), 0, 'f', 1).arg(World::UNIT_LABEL);
    }

    if (const auto velocity = cell.getVelocity(); velocity.length() > 0.01F) {
        label += QString(" | Prędkość wody: (%1, %2) m/s")
            .arg(World::toDisplay(velocity.x()), 0, 'f', 1)
            .arg(World::toDisplay(velocity.y()), 0, 'f', 1);
    }

    if (cell.getType() == OBSTACLE) {
        label += QString(" | Przeszkoda");
    }
    if (cell.getType() == RIVER) {
        label += QString(" | Rzeka");
    }
    if (cell.getType() == WATER_SOURCE) {
        label += QString(" | Źródło wody (siła: %1 %2)")
            .arg(World::toDisplay(cell.getSourceStrength()), 0, 'f', 1)
            .arg(World::UNIT_LABEL);
    }
    if (cell.isRainArea()) {
        label += QString(" | Deszcz (intensywność: %1)").arg(cell.getRainIntensity(), 0, 'f', 2);
    }
    if (cell.getWaterDepth() > cell.getRiverCapacity()) {
        label += QString(" | Przelew wody!");
    }

    cellInfoLabel->setText(QString("Komórka: (%1, %2) | Pozycja: (%3, %4) %5 | %6")
        .arg(gridX).arg(gridY)
        .arg(worldX, 0, 'f', 1).arg(worldY, 0, 'f', 1)
        .arg(World::UNIT_LABEL)
        .arg(label));
}

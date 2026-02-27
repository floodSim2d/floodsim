#include "SimulationToolbar.h"

#include <QAction>
#include <QInputDialog>
#include <QLabel>

#include "../Simulation/Grid/Grid.h"
#include "../Simulation/Grid/Cell.h"
#include "../Simulation/FlowModel/FlowModel.h"
#include "../Renderer/OpenGLRenderer.h"

SimulationToolbar::SimulationToolbar(Grid* grid, FlowModel* flowModel, OpenGLRenderer* renderer, QWidget* parent)
    : QToolBar(parent),
      grid(grid),
      flowModel(flowModel),
      renderer(renderer),
      cellInfoLabel(nullptr)
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
        emit statusMessageRequested("Symulacja zresetowana");
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
    QString label = QString("Wysokość terenu: %1").arg(cell.getTerrainHeight(), 0, 'f', 2);

    if (const auto waterDepth = cell.getWaterDepth(); waterDepth > 0.0F) {
        label += QString(" | Głębokość wody: %1").arg(waterDepth, 0, 'f', 2);
        label += QString(" | Całkowita wysokość: %1").arg(cell.getTotalHeight(), 0, 'f', 2);
    }

    if (const auto velocity = cell.getVelocity(); velocity.length() > 0.01F) {
        label += QString(" | Prędkość wody: (%1, %2)")
            .arg(velocity.x(), 0, 'f', 2)
            .arg(velocity.y(), 0, 'f', 2);
    }

    if (cell.getType() == OBSTACLE) {
        label += QString(" | Przeszkoda");
    }
    if (cell.getType() == RIVER) {
        label += QString(" | Rzeka");
    }
    if (cell.getType() == WATER_SOURCE) {
        label += QString(" | Źródło wody (siła: %1)").arg(cell.getSourceStrength(), 0, 'f', 2);
    }
    if (cell.isRainArea()) {
        label += QString(" | Deszcz (intensywność: %1)").arg(cell.getRainIntensity(), 0, 'f', 2);
    }
    if (cell.getWaterDepth() > cell.getRiverCapacity()) {
        label += QString(" | Przelew wody!");
    }

    cellInfoLabel->setText(QString("Pozycja: (%1, %2) | %3").arg(gridX).arg(gridY).arg(label));
}

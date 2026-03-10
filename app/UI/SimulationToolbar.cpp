#include "SimulationToolbar.h"

#include <QAction>
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
      positionLabel(nullptr),
      terrainLabel(nullptr),
      waterLabel(nullptr),
      velocityLabel(nullptr),
      typeLabel(nullptr)
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
    QAction* resetAction = addAction("🔄 Reset");

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
}

void SimulationToolbar::setupCellInfoDisplay() {
    addSeparator();

    const QString labelStyle =
        "QLabel { padding: 3px 6px; background-color: rgba(0, 0, 0, 0.08); border-radius: 3px; font-family: monospace; }";

    positionLabel = new QLabel("Pozycja: ---", this);
    positionLabel->setStyleSheet(labelStyle);
    positionLabel->setFixedWidth(180);
    addWidget(positionLabel);

    terrainLabel = new QLabel("Teren: ---", this);
    terrainLabel->setStyleSheet(labelStyle);
    terrainLabel->setFixedWidth(140);
    addWidget(terrainLabel);

    waterLabel = new QLabel("Woda: ---", this);
    waterLabel->setStyleSheet(labelStyle);
    waterLabel->setFixedWidth(140);
    addWidget(waterLabel);

    velocityLabel = new QLabel("V: ---", this);
    velocityLabel->setStyleSheet(labelStyle);
    velocityLabel->setFixedWidth(250);
    addWidget(velocityLabel);

    typeLabel = new QLabel("Typ: ---", this);
    typeLabel->setStyleSheet(labelStyle);
    typeLabel->setFixedWidth(160);
    addWidget(typeLabel);

    connect(renderer, &OpenGLRenderer::cellHovered, this, &SimulationToolbar::updateCellInfo);
}

void SimulationToolbar::updateCellInfo(int gridX, int gridY, const Cell& cell) {
    const float cs = grid->getCellSize();
    const float worldX = World::toDisplay(static_cast<float>(gridX) * cs);
    const float worldY = World::toDisplay(static_cast<float>(gridY) * cs);

    positionLabel->setText(QString("(%1, %2) %3")
        .arg(worldX, 0, 'f', 0).arg(worldY, 0, 'f', 0).arg(World::UNIT_LABEL));

    terrainLabel->setText(QString("Teren: %1 %2")
        .arg(World::toDisplay(cell.getTerrainHeight()), 0, 'f', 1)
        .arg(World::UNIT_LABEL));

    if (cell.getWaterDepth() > 0.01F) {
        waterLabel->setText(QString("Woda: %1 %2")
            .arg(World::toDisplay(cell.getWaterDepth()), 0, 'f', 1)
            .arg(World::UNIT_LABEL));
    } else {
        waterLabel->setText("Woda: ---");
    }

    if (const auto velocity = cell.getVelocity(); velocity.length() > 0.01F) {
        velocityLabel->setText(QString("V: (%1, %2) |%3| %4/s")
            .arg(World::toDisplay(velocity.x()), 0, 'f', 1)
            .arg(World::toDisplay(velocity.y()), 0, 'f', 1)
            .arg(World::toDisplay(velocity.length()), 0, 'f', 2)
            .arg(World::UNIT_LABEL));
    } else {
        velocityLabel->setText("V: ---");
    }

    QString typeStr;
    switch (cell.getType()) {
        case OBSTACLE:     typeStr = "Przeszkoda"; break;
        case RIVER:        typeStr = "Rzeka"; break;
        case WATER_SOURCE: typeStr = QString("Zrodlo (%1 %2)")
            .arg(World::toDisplay(cell.getSourceStrength()), 0, 'f', 1)
            .arg(World::UNIT_LABEL); break;
        case LAND:         typeStr = "Teren"; break;
        default:           typeStr = "---"; break;
    }
    if (cell.getWaterDepth() > cell.getRiverCapacity() && cell.getType() == RIVER) {
        typeStr += " [przelew!]";
    }
    typeLabel->setText(typeStr);
}

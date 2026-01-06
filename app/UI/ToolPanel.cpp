#include "ToolPanel.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QButtonGroup>

#include "../Simulation/Tools/PaintTool.h"

ToolPanel::ToolPanel(PaintTool* paintTool, QWidget* parent)
    : QWidget(parent),
      paintTool(paintTool),
      brushSizeSlider(nullptr),
      brushSizeValueLabel(nullptr)
{
    setupUI();
}

void ToolPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Narzędzia:", this));
    layout->addSpacing(10);

    setupToolButtons();
    layout->addSpacing(10);
    setupBrushControls();
    layout->addStretch();
}

void ToolPanel::setupToolButtons() {
    auto* buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);

    struct ToolButtonInfo {
        QString name;
        ToolType type;
        QString message;
    };

    std::vector<ToolButtonInfo> tools = {
        {"Kamera", ToolType::Camera, "Tryb kamery włączony - nawiguj sceną 3D"},
        {"Teren", ToolType::Terrain, "Narzędzie: Teren - kliknij aby podnieść teren"},
        {"Rzeka", ToolType::River, "Narzędzie: Rzeka - kliknij aby utworzyć rzekę"},
        {"Źródło wody", ToolType::WaterSource, "Narzędzie: Źródło wody - stałe źródło utrzymujące poziom wody"},
        {"Gumka", ToolType::Eraser, "Narzędzie: Gumka - kliknij aby wyczyścić komórkę"}
    };

    auto* layout = qobject_cast<QVBoxLayout*>(this->layout());

    for (const auto& toolInfo : tools) {
        auto* button = new QPushButton(toolInfo.name, this);
        button->setCheckable(true);
        button->setStyleSheet(
            "QPushButton { padding: 8px; font-weight: bold; }"
            "QPushButton:checked { background-color: #4CAF50; color: white; }"
        );
        layout->addWidget(button);
        buttonGroup->addButton(button);

        connect(button, &QPushButton::clicked, this, [this, toolInfo]() {
            paintTool->setToolType(toolInfo.type);
            bool isCameraMode = (toolInfo.type == ToolType::Camera);
            emit toolSelected(toolInfo.message, isCameraMode);
        });
    }

    // default button is terrain
    buttonGroup->buttons().at(1)->setChecked(true);
    paintTool->setToolType(tools[1].type);
    emit toolSelected(tools[1].message, false);
}

void ToolPanel::setupBrushControls() {
    auto* layout = qobject_cast<QVBoxLayout*>(this->layout());

    auto* brushSizeLabel = new QLabel("Rozmiar pędzla:", this);
    layout->addWidget(brushSizeLabel);

    brushSizeSlider = new QSlider(Qt::Horizontal, this);
    brushSizeSlider->setMinimum(1);
    brushSizeSlider->setMaximum(10);
    brushSizeSlider->setValue(1);
    brushSizeSlider->setTickPosition(QSlider::TicksBelow);
    brushSizeSlider->setTickInterval(1);
    layout->addWidget(brushSizeSlider);

    brushSizeValueLabel = new QLabel("1", this);
    brushSizeValueLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(brushSizeValueLabel);

    connect(brushSizeSlider, &QSlider::valueChanged, this, [this](int value) {
        paintTool->setBrushSize(value);
        brushSizeValueLabel->setText(QString::number(value));
    });
}


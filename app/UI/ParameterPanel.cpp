#include "ParameterPanel.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QCheckBox>

#include "../Simulation/Grid/Grid.h"
#include "../Simulation/FlowModel/FlowModel.h"
#include "../Renderer/OpenGLRenderer.h"
#include "../WorldConstants.h"

ParameterPanel::ParameterPanel(Grid* grid, FlowModel* flowModel, OpenGLRenderer* renderer, QWidget* parent)
    : QWidget(parent),
      grid(grid),
      flowModel(flowModel),
      renderer(renderer),
      flowCoefficientSpinBox(nullptr),
      maxDepthSlider(nullptr),
      depthValueLabel(nullptr),
      infiltrationSlider(nullptr),
      infiltrationValueLabel(nullptr)
{
    setAutoFillBackground(true);
    setupUI();
}

void ParameterPanel::setupUI() {
    auto* panelLayout = new QVBoxLayout(this);

    auto* groupBox = new QGroupBox("Parametry", this);
    auto* groupLayout = new QVBoxLayout(groupBox);

    flowCoefficientSpinBox = new QDoubleSpinBox(groupBox);
    flowCoefficientSpinBox->setRange(0, 100);
    flowCoefficientSpinBox->setValue(1);

    groupLayout->addWidget(new QLabel("K:", groupBox));
    groupLayout->addWidget(flowCoefficientSpinBox);

    groupLayout->addWidget(new QLabel("Max głębokość (m):", groupBox));

    maxDepthSlider = new QSlider(Qt::Horizontal, groupBox);
    maxDepthSlider->setMinimum(static_cast<int>(World::toDisplay(World::MIN_WATER_DEPTH)));
    maxDepthSlider->setMaximum(static_cast<int>(World::toDisplay(World::MAX_WATER_DEPTH)));
    maxDepthSlider->setValue(static_cast<int>(World::toDisplay(World::DEFAULT_WATER_DEPTH)));
    maxDepthSlider->setSingleStep(10);
    maxDepthSlider->setTickPosition(QSlider::TicksBelow);
    maxDepthSlider->setTickInterval(100);
    groupLayout->addWidget(maxDepthSlider);

    depthValueLabel = new QLabel(QString("%1 %2")
        .arg(maxDepthSlider->value())
        .arg(World::UNIT_LABEL), groupBox);
    depthValueLabel->setAlignment(Qt::AlignCenter);
    groupLayout->addWidget(depthValueLabel);

    auto* applyButton = new QPushButton("Zastosuj", groupBox);
    groupLayout->addWidget(applyButton);

    panelLayout->addWidget(groupBox);

    auto* weatherGrp = new QGroupBox("Pogoda", this);
    auto* weatherLayout = new QVBoxLayout(weatherGrp);

    auto* rainInfo = new QLabel("Symuluje opady na całej powierzchni mapy. Suwak określa przyrost wody w metrach na sekundę (max 0.5 m/s).", weatherGrp);
    rainInfo->setWordWrap(true);
    rainInfo->setStyleSheet("QLabel { color: #666; font-size: 11px; margin-bottom: 5px; }");
    weatherLayout->addWidget(rainInfo);

    auto* rainCheck = new QCheckBox("Globalny deszcz", weatherGrp);
    weatherLayout->addWidget(rainCheck);

    weatherLayout->addWidget(new QLabel("Intensywność opadów:", weatherGrp));
    auto* rainSlider = new QSlider(Qt::Horizontal, weatherGrp);
    rainSlider->setMinimum(0);
    rainSlider->setMaximum(100);
    rainSlider->setValue(0);
    weatherLayout->addWidget(rainSlider);

    auto* rainValueLabel = new QLabel("0.0", weatherGrp);
    rainValueLabel->setAlignment(Qt::AlignCenter);
    weatherLayout->addWidget(rainValueLabel);

    panelLayout->addWidget(weatherGrp);

    // --- Osobna grupa: Gleba ---
    auto* soilGrp = new QGroupBox("Gleba", this);
    auto* soilLayout = new QVBoxLayout(soilGrp);

    auto* infiltrationInfo = new QLabel("Wsiąkanie wody w ziemię (nie dotyczy rzek ani źródeł wody). Suwak określa ubytek wody w m/s.", soilGrp);
    infiltrationInfo->setWordWrap(true);
    infiltrationInfo->setStyleSheet("QLabel { color: #666; font-size: 11px; margin-bottom: 5px; }");
    soilLayout->addWidget(infiltrationInfo);

    soilLayout->addWidget(new QLabel("Współczynnik wsiąkania:", soilGrp));

    infiltrationSlider = new QSlider(Qt::Horizontal, soilGrp);
    infiltrationSlider->setMinimum(0);
    infiltrationSlider->setMaximum(100);
    infiltrationSlider->setValue(0);
    soilLayout->addWidget(infiltrationSlider);

    infiltrationValueLabel = new QLabel("0.000", soilGrp);
    infiltrationValueLabel->setAlignment(Qt::AlignCenter);
    soilLayout->addWidget(infiltrationValueLabel);

    panelLayout->addWidget(soilGrp);
    panelLayout->addStretch();

    connect(maxDepthSlider, &QSlider::valueChanged, this, [this](int value) {
        depthValueLabel->setText(QString("%1 %2")
            .arg(value)
            .arg(World::UNIT_LABEL));
    });

    connect(applyButton, &QPushButton::clicked, this, &ParameterPanel::applyParameters);

    connect(rainCheck, &QCheckBox::toggled, this, [this](bool checked) {
        flowModel->setGlobalRainEnabled(checked);
        QString message = checked ? "Włączono globalne opady deszczu" : "Wyłączono opady deszczu";
        emit parametersApplied(message);
    });

    connect(rainSlider, &QSlider::valueChanged, this, [this, rainValueLabel](int value) {
        float intensity = static_cast<float>(value) / 200.0f;
        flowModel->setGlobalRainIntensity(intensity);
        rainValueLabel->setText(QString::number(intensity, 'f', 3));
    });

    connect(infiltrationSlider, &QSlider::valueChanged, this, [this](int value) {
        float rate = static_cast<float>(value) / 200.0f;
        flowModel->setInfiltrationRate(rate);
        infiltrationValueLabel->setText(QString::number(rate, 'f', 3));
    });
}

void ParameterPanel::applyParameters() {
    const auto kValue = static_cast<float>(flowCoefficientSpinBox->value());
    flowModel->setPipeFriction(kValue);

    // Slider stores display values (meters); convert back to internal units
    const float displayDepth = static_cast<float>(maxDepthSlider->value());
    const float internalDepth = displayDepth / World::DISPLAY_SCALE;
    grid->setMaxDepth(internalDepth);
    renderer->updateProjectionMatrix();

    QString message = QString("Parametry zastosowane: K=%1, Max głębokość=%2 %3")
        .arg(kValue, 0, 'f', 2)
        .arg(displayDepth, 0, 'f', 0)
        .arg(World::UNIT_LABEL);

    emit parametersApplied(message);
}

#include "ParameterPanel.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>

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
      simSpeedSlider(nullptr),
      simSpeedValueLabel(nullptr),
      infiltrationSlider(nullptr),
      infiltrationValueLabel(nullptr)
{
    setAutoFillBackground(true);
    setupUI();
}

void ParameterPanel::setupUI() {
    auto* panelLayout = new QVBoxLayout(this);

    // ─── Hydraulika ────────────────────────────────────────────────────
    auto* flowGrp = new QGroupBox("Hydraulika", this);
    auto* flowLayout = new QVBoxLayout(flowGrp);

    auto* frictionInfo = new QLabel(
        "Kontroluje jak szybko woda się rozpływa. "
        "Niska wartość = szybki przepływ, wysoka = wolny.",
        flowGrp);
    frictionInfo->setWordWrap(true);
    frictionInfo->setStyleSheet("QLabel { color: #666; font-size: 11px; margin-bottom: 5px; }");
    flowLayout->addWidget(frictionInfo);

    flowLayout->addWidget(new QLabel("Opór przepływu:", flowGrp));

    flowCoefficientSpinBox = new QDoubleSpinBox(flowGrp);
    flowCoefficientSpinBox->setRange(0.0, 10.0);
    flowCoefficientSpinBox->setSingleStep(0.1);
    flowCoefficientSpinBox->setDecimals(2);
    flowCoefficientSpinBox->setValue(0.5);
    flowCoefficientSpinBox->setToolTip("0 = woda płynie swobodnie, 2+ = bardzo wolny przepływ");
    flowLayout->addWidget(flowCoefficientSpinBox);

    flowLayout->addWidget(new QLabel("Max głębokość wody:", flowGrp));

    maxDepthSlider = new QSlider(Qt::Horizontal, flowGrp);
    maxDepthSlider->setMinimum(static_cast<int>(World::toDisplay(World::MIN_WATER_DEPTH)));
    maxDepthSlider->setMaximum(static_cast<int>(World::toDisplay(World::MAX_WATER_DEPTH)));
    maxDepthSlider->setValue(static_cast<int>(World::toDisplay(World::DEFAULT_WATER_DEPTH)));
    maxDepthSlider->setSingleStep(10);
    maxDepthSlider->setTickPosition(QSlider::TicksBelow);
    maxDepthSlider->setTickInterval(100);
    flowLayout->addWidget(maxDepthSlider);

    depthValueLabel = new QLabel(QString("%1 %2")
        .arg(maxDepthSlider->value())
        .arg(World::UNIT_LABEL), flowGrp);
    depthValueLabel->setAlignment(Qt::AlignCenter);
    flowLayout->addWidget(depthValueLabel);

    // ─── Prędkość symulacji ────────────────────────────────────────────
    flowLayout->addWidget(new QLabel("Prędkość symulacji:", flowGrp));

    simSpeedSlider = new QSlider(Qt::Horizontal, flowGrp);
    simSpeedSlider->setMinimum(1);
    simSpeedSlider->setMaximum(100);
    simSpeedSlider->setValue(16);
    simSpeedSlider->setToolTip("Mniejsza wartość = szybsza symulacja");
    flowLayout->addWidget(simSpeedSlider);

    simSpeedValueLabel = new QLabel("16 ms (~60 fps)", flowGrp);
    simSpeedValueLabel->setAlignment(Qt::AlignCenter);
    flowLayout->addWidget(simSpeedValueLabel);

    auto* applyButton = new QPushButton("Zastosuj", flowGrp);
    flowLayout->addWidget(applyButton);

    panelLayout->addWidget(flowGrp);

    // ─── Pogoda ────────────────────────────────────────────────────────
    auto* weatherGrp = new QGroupBox("Pogoda", this);
    auto* weatherLayout = new QVBoxLayout(weatherGrp);

    auto* rainInfo = new QLabel(
        "Dodaje wodę na całą mapę, symulując opady deszczu.",
        weatherGrp);
    rainInfo->setWordWrap(true);
    rainInfo->setStyleSheet("QLabel { color: #666; font-size: 11px; margin-bottom: 5px; }");
    weatherLayout->addWidget(rainInfo);

    auto* rainToggle = new QPushButton("Deszcz wyłączony", weatherGrp);
    rainToggle->setCheckable(true);
    rainToggle->setChecked(false);
    rainToggle->setStyleSheet(
        "QPushButton { padding: 6px; font-weight: bold; }"
        "QPushButton:checked { background-color: #2196F3; color: white; }"
    );
    weatherLayout->addWidget(rainToggle);

    weatherLayout->addWidget(new QLabel("Intensywność opadów:", weatherGrp));
    auto* rainSlider = new QSlider(Qt::Horizontal, weatherGrp);
    rainSlider->setMinimum(0);
    rainSlider->setMaximum(100);
    rainSlider->setValue(0);
    weatherLayout->addWidget(rainSlider);

    auto* rainValueLabel = new QLabel("0.000 m/s", weatherGrp);
    rainValueLabel->setAlignment(Qt::AlignCenter);
    weatherLayout->addWidget(rainValueLabel);

    panelLayout->addWidget(weatherGrp);

    // ─── Gleba ─────────────────────────────────────────────────────────
    auto* soilGrp = new QGroupBox("Gleba", this);
    auto* soilLayout = new QVBoxLayout(soilGrp);

    auto* infiltrationInfo = new QLabel(
        "Woda wsiąka w ziemię (nie dotyczy rzek i źródeł).",
        soilGrp);
    infiltrationInfo->setWordWrap(true);
    infiltrationInfo->setStyleSheet("QLabel { color: #666; font-size: 11px; margin-bottom: 5px; }");
    soilLayout->addWidget(infiltrationInfo);

    soilLayout->addWidget(new QLabel("Współczynnik wsiąkania:", soilGrp));

    infiltrationSlider = new QSlider(Qt::Horizontal, soilGrp);
    infiltrationSlider->setMinimum(0);
    infiltrationSlider->setMaximum(100);
    infiltrationSlider->setValue(0);
    soilLayout->addWidget(infiltrationSlider);

    infiltrationValueLabel = new QLabel("0.000 m/s", soilGrp);
    infiltrationValueLabel->setAlignment(Qt::AlignCenter);
    soilLayout->addWidget(infiltrationValueLabel);

    panelLayout->addWidget(soilGrp);
    panelLayout->addStretch();

    connect(flowCoefficientSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        flowModel->setPipeFriction(static_cast<float>(value));
    });

    connect(maxDepthSlider, &QSlider::valueChanged, this, [this](int value) {
        depthValueLabel->setText(QString("%1 %2")
            .arg(value)
            .arg(World::UNIT_LABEL));
    });

    connect(simSpeedSlider, &QSlider::valueChanged, this, [this](int value) {
        flowModel->setUpdateInterval(value);
        int approxFps = 1000 / std::max(1, value);
        simSpeedValueLabel->setText(QString("%1 ms (~%2 fps)")
            .arg(value).arg(approxFps));
    });

    connect(applyButton, &QPushButton::clicked, this, &ParameterPanel::applyParameters);

    connect(rainToggle, &QPushButton::toggled, this, [this, rainToggle](bool checked) {
        flowModel->setGlobalRainEnabled(checked);
        rainToggle->setText(checked ? "Deszcz włączony" : "Deszcz wyłączony");
        QString message = checked ? "Włączono globalne opady deszczu" : "Wyłączono opady deszczu";
        emit parametersApplied(message);
    });

    connect(rainSlider, &QSlider::valueChanged, this, [this, rainValueLabel](int value) {
        float intensity = static_cast<float>(value) / 200.0F;
        flowModel->setGlobalRainIntensity(intensity);
        rainValueLabel->setText(QString("%1 m/s").arg(intensity, 0, 'f', 3));
    });

    connect(infiltrationSlider, &QSlider::valueChanged, this, [this](int value) {
        float rate = static_cast<float>(value) / 200.0F;
        flowModel->setInfiltrationRate(rate);
        infiltrationValueLabel->setText(QString("%1 m/s").arg(rate, 0, 'f', 3));
    });
}

void ParameterPanel::applyParameters() {
    const auto frictionValue = static_cast<float>(flowCoefficientSpinBox->value());
    flowModel->setPipeFriction(frictionValue);

    // Slider stores display values (meters); convert back to internal units
    const float displayDepth = static_cast<float>(maxDepthSlider->value());
    const float internalDepth = displayDepth / World::DISPLAY_SCALE;
    grid->setMaxDepth(internalDepth);
    renderer->updateProjectionMatrix();

    QString message = QString("Parametry zastosowane: opór=%1, max głębokość=%2 %3")
        .arg(frictionValue, 0, 'f', 2)
        .arg(displayDepth, 0, 'f', 0)
        .arg(World::UNIT_LABEL);

    emit parametersApplied(message);
}

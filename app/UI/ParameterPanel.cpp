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

ParameterPanel::ParameterPanel(Grid* grid, FlowModel* flowModel, OpenGLRenderer* renderer, QWidget* parent)
    : QWidget(parent),
      grid(grid),
      flowModel(flowModel),
      renderer(renderer),
      flowCoefficientSpinBox(nullptr),
      maxDepthSlider(nullptr),
      depthValueLabel(nullptr)
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

    groupLayout->addWidget(new QLabel("Max głębokość:", groupBox));

    maxDepthSlider = new QSlider(Qt::Horizontal, groupBox);
    maxDepthSlider->setMinimum(MIN_WATER_DEPTH);
    maxDepthSlider->setMaximum(MAX_WATER_DEPTH);
    maxDepthSlider->setValue(DEFAULT_WATER_DEPTH);
    maxDepthSlider->setTickPosition(QSlider::TicksBelow);
    maxDepthSlider->setTickInterval(10);
    groupLayout->addWidget(maxDepthSlider);

    depthValueLabel = new QLabel(QString::number(maxDepthSlider->value()), groupBox);
    depthValueLabel->setAlignment(Qt::AlignCenter);
    groupLayout->addWidget(depthValueLabel);

    auto* applyButton = new QPushButton("Zastosuj", groupBox);
    groupLayout->addWidget(applyButton);

    panelLayout->addWidget(groupBox);
    panelLayout->addStretch();

    connect(maxDepthSlider, &QSlider::valueChanged, this, [this](int value) {
        depthValueLabel->setText(QString::number(value));
    });

    connect(applyButton, &QPushButton::clicked, this, &ParameterPanel::applyParameters);
}

void ParameterPanel::applyParameters() {
    const auto kValue = static_cast<float>(flowCoefficientSpinBox->value());
    flowModel->setFlowCoefficient(kValue);

    const auto newDepth = static_cast<float>(maxDepthSlider->value());
    grid->setMaxDepth(newDepth);
    renderer->updateProjectionMatrix();

    QString message = QString("Parametry zastosowane: K=%1, Max głębokość=%2")
        .arg(kValue, 0, 'f', 2)
        .arg(newDepth, 0, 'f', 1);

    emit parametersApplied(message);
}


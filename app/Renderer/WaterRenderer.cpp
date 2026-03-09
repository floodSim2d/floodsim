#include "WaterRenderer.h"

#include "../Simulation/Grid/Grid.h"
#include "../Utils/Logger.h"

WaterRenderer::WaterRenderer(Grid* grid)
    : grid(grid),
      glContext(nullptr),
      shaderProgram(nullptr),
      vertexBuffer(nullptr),
      indexBuffer(nullptr),
      VAO(nullptr),
      indexCount(0),
      meshResolution(200) {
}

WaterRenderer::~WaterRenderer() {
    LOG("WaterRenderer destructor");
    shaderProgram = nullptr;
    vertexBuffer = nullptr;
    indexBuffer = nullptr;
    VAO = nullptr;
}

void WaterRenderer::initialize(QOpenGLFunctions* gl_context) {
    glContext = gl_context;
    meshResolution = std::min(static_cast<unsigned int>(
        std::max(grid->getWidth(), grid->getHeight())), 400u);

    createShaders();
    createMesh();

    LOG("WaterRenderer initialized.");
}

void WaterRenderer::createShaders() {
    shaderProgram = new QOpenGLShaderProgram();

    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Renderer/shaders/water.vert");
    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Renderer/shaders/water.frag");
    shaderProgram->link();
    if (!shaderProgram->isLinked()) {
        LOG(QString("Water Shader Program linking failed: %1").arg(shaderProgram->log()));
    }
}

void WaterRenderer::createMesh() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int y = 0; y <= meshResolution; y++) {
        for (unsigned int x = 0; x <= meshResolution; x++) {
            float posX = static_cast<float>(x) / static_cast<float>(meshResolution);
            float posY = static_cast<float>(y) / static_cast<float>(meshResolution);

            // vertex pos
            vertices.push_back(posX);
            vertices.push_back(posY);

            // tex coords
            vertices.push_back(posX);
            vertices.push_back(posY);
        }
    }

    // indices
    for (unsigned int y = 0; y < meshResolution; y++) {
        for (unsigned int x = 0; x < meshResolution; x++) {
            const unsigned int topLeft = y * (meshResolution + 1) + x;
            const unsigned int topRight = topLeft + 1;
            const unsigned int bottomLeft = (y + 1) * (meshResolution + 1) + x;
            const unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    indexCount = static_cast<unsigned int>(indices.size());

    VAO = new QOpenGLVertexArrayObject();
    VAO->create();
    VAO->bind();

    vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vertexBuffer->create();
    vertexBuffer->bind();
    vertexBuffer->allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));

    glContext->glEnableVertexAttribArray(0);
    glContext->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    glContext->glEnableVertexAttribArray(1);
    glContext->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                     reinterpret_cast<void*>(2 * sizeof(float)));

    indexBuffer = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    indexBuffer->create();
    indexBuffer->bind();
    indexBuffer->allocate(indices.data(), static_cast<int>(indices.size() * sizeof(unsigned int)));

    VAO->release();
}

void WaterRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view,
                            QOpenGLTexture* heightMapTexture, const QVector3D& viewPos) const {
    if (shaderProgram == nullptr || heightMapTexture == nullptr) {
        return;
    }

    shaderProgram->bind();

    glContext->glActiveTexture(GL_TEXTURE0);
    heightMapTexture->bind();

    shaderProgram->setUniformValue("projection", projection);
    shaderProgram->setUniformValue("view", view);
    shaderProgram->setUniformValue("heightMap", 0);
    shaderProgram->setUniformValue("gridSize",
        QVector2D(static_cast<float>(grid->getWidth()),
                  static_cast<float>(grid->getHeight())));
    shaderProgram->setUniformValue("cellSize", grid->getCellSize());

    shaderProgram->setUniformValue("lightDirection", QVector3D(0.4F, 0.3F, 0.8F).normalized());
    shaderProgram->setUniformValue("viewPos", viewPos);

    // render water with alpha blending on top of terrain
    glContext->glDepthMask(GL_FALSE);

    VAO->bind();
    glContext->glDrawElements(GL_TRIANGLES, static_cast<int>(indexCount), GL_UNSIGNED_INT, nullptr);
    VAO->release();

    glContext->glDepthMask(GL_TRUE);

    heightMapTexture->release();
    shaderProgram->release();
}


void WaterRenderer::cleanup() {
    LOG("WaterRenderer::cleanup() - start");

    if (!glContext) {
        LOG("WaterRenderer::cleanup() - no GL context, skipping");
        return;
    }

    if (VAO && VAO->isCreated()) {
        VAO->destroy();
        delete VAO;
        VAO = nullptr;
    }

    if (indexBuffer && indexBuffer->isCreated()) {
        indexBuffer->destroy();
        delete indexBuffer;
        indexBuffer = nullptr;
    }

    if (vertexBuffer && vertexBuffer->isCreated()) {
        vertexBuffer->destroy();
        delete vertexBuffer;
        vertexBuffer = nullptr;
    }

    if (shaderProgram) {
        delete shaderProgram;
        shaderProgram = nullptr;
    }

    LOG("WaterRenderer::cleanup() - end");
}


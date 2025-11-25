#include "Grid.h"

Grid::Grid(int width, int height, float cellSize)
    : glContext(nullptr),
      shaderProgram(nullptr),
      vertexBuffer(nullptr),
      indexBuffer(nullptr),
      VAO(nullptr),
      heightTexture(nullptr),
      indexCount(0),
      meshResolution(200),
      width(width),
      height(height),
      cellSize(cellSize) {
    heightMap.resize(width * height, 0.0f);
}

Grid::~Grid() {
    delete shaderProgram;
    delete vertexBuffer;
    delete indexBuffer;
    delete VAO;
    delete heightTexture;
}

void Grid::initialize(QOpenGLFunctions_3_3_Core* gl_context) {
    glContext = gl_context;

    createShaders();
    createMesh();
    createHeightTexture();
}

void Grid::createShaders() {
    shaderProgram = new QOpenGLShaderProgram();

    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Simulation/shaders/grid_vertex.glsl");
    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Simulation/shaders/grid_fragment.glsl");
    shaderProgram->link();
    if (!shaderProgram->isLinked()) {
        qDebug() << "Shader Program linking failed" << shaderProgram->log();
    }
}

void Grid::createMesh() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Generate vertices
    for (int y = 0; y <= meshResolution; y++) {
        for (int x = 0; x <= meshResolution; x++) {
            float posX = x / static_cast<float>(meshResolution);
            float posY = y / static_cast<float>(meshResolution);

            vertices.push_back(posX);
            vertices.push_back(posY);

            // tex coords, posZ is 0
            vertices.push_back(posX);
            vertices.push_back(posY);
        }
    }

    // generate indices
    for (int y = 0; y < meshResolution; y++) {
        for (int x = 0; x < meshResolution; x++) {
            /*
             * meshResolution = 2, liczymy razem z 0 wiec wyjdzie ze 3 wymiary
             * 0-1-2
             * 3-4-5
             * 6-7-8
             *
             * dla 0
             * topLeft = 0 * 3 + 0 = 0
             * topRight = 0 * 3 + 1 = 1
             * bottomLeft = 1 * 3 + 0 = 3
             * bottomRight = 1 * 3 + 1 = 4
             */
            int topLeft = y * (meshResolution + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * (meshResolution + 1) + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    indexCount = indices.size();

    VAO = new QOpenGLVertexArrayObject();
    VAO->create();
    VAO->bind();

    vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vertexBuffer->create();
    vertexBuffer->bind();
    vertexBuffer->allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));

    glContext->glEnableVertexAttribArray(0);
    glContext->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glContext->glEnableVertexAttribArray(1);
    glContext->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    indexBuffer = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    indexBuffer->create();
    indexBuffer->bind();
    indexBuffer->allocate(indices.data(), static_cast<int>(indices.size() * sizeof(unsigned int)));

    VAO->release();
}

void Grid::createHeightTexture() {
    heightTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    heightTexture->create();
    heightTexture->setSize(width, height);
    heightTexture->setFormat(QOpenGLTexture::RGB32F);
    heightTexture->allocateStorage();

    heightTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    heightTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    heightTexture->setWrapMode(QOpenGLTexture::WrapMode::ClampToEdge);

    updateHeightTexture();
}

void Grid::updateHeightTexture() {
    if (heightTexture == nullptr) {
        qDebug() << "heightTexture is null";
        return;
    }
    heightTexture->bind();
    glContext->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, heightMap.data());
    heightTexture->generateMipMaps();
}

void Grid::render(const QMatrix4x4 projection, const QMatrix4x4 view) {
    if (shaderProgram == nullptr) {
        qDebug() << "Shader Program not created, cannot render grid.";
        return;
    }
    shaderProgram->bind();

    glContext->glActiveTexture(GL_TEXTURE0);
    heightTexture->bind();

    shaderProgram->setUniformValue("view", view);
    shaderProgram->setUniformValue("projection", projection);
    shaderProgram->setUniformValue("heightMap", 0);
    shaderProgram->setUniformValue("gridSize", QVector2D(width, height));
    shaderProgram->setUniformValue("heightScale", 1.0F);

    VAO->bind();
    glContext->glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    VAO->release();

    shaderProgram->release();
}

bool Grid::isValidPosition(int x, int y) const { return x >= 0 && x < width && y >= 0 && y < height; }

float Grid::getHeightValue(int x, int y) const {
    if (!isValidPosition(x, y)) return 0.0f;

    return heightMap[y * width + x];
}

void Grid::setHeightValue(int x, int y, float height) {
    if (!isValidPosition(x, y)) {
        return;
    }

    heightMap[y * width + x] = height;
}

auto Grid::worldPosToGrid(const QVector2D& worldPos) const -> QVector2D {
    float gridX = worldPos.x() / cellSize;
    float gridY = worldPos.y() / cellSize;

    return {gridX, gridY};
}

void Grid::saveHeightmap(const QString& filename) const {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open file for writing:" << filename;
        return;
    }

    QDataStream stream(&file);
    stream << width << height;

    for (float heightVal : heightMap) {
        stream << heightVal;
    }

    file.close();
}

void Grid::loadHeightmap(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open file for reading:" << filename;
        return;
    }

    QDataStream stream(&file);
    int file_width, file_height;
    stream >> file_width >> file_height;
    const bool needResize = file_width != width || file_height != height;
    if (needResize) {
        width = file_width;
        height = file_height;
        heightMap.resize(width * height);
        delete heightTexture;
        heightTexture = nullptr;
    }

    for (int i = 0; i < heightMap.size(); i++) {
        stream >> heightMap[i];
    }

    if (needResize) {
        createHeightTexture();
    } else {
        updateHeightTexture();
    }
    file.close();
}

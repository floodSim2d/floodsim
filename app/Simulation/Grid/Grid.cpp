#include "Grid.h"

#include <QFile>

Grid::Grid(const int width, const int height, const float cellSize, const float maxDepth)
    : glContext(nullptr),
      shaderProgram(nullptr),
      vertexBuffer(nullptr),
      indexBuffer(nullptr),
      VAO(nullptr),
      heightTexture(nullptr),
      width(width),
      height(height),
      cellSize(cellSize),
      maxDepth(maxDepth),
      indexCount(0),
      meshResolution(200) { // TODO: replace with parameter or something that isnt magic number
    heightMap.resize(width * height, Cell());
}

Grid::~Grid() {
    VAO->destroy();
    indexBuffer->destroy();
    vertexBuffer->destroy();
    heightTexture->destroy();

    delete shaderProgram;
    delete vertexBuffer;
    delete indexBuffer;
    delete VAO;
    delete heightTexture;
}

void Grid::initialize(QOpenGLFunctions* gl_context) {
    glContext = gl_context;

    createShaders();
    createMesh();
    createHeightTexture();

    // Create realistic terrain with land, water, hills, and valleys
    // for (int y = 0; y < height; ++y) {
    //     for (int x = 0; x < width; ++x) {
    //         const float nx = static_cast<float>(x) / static_cast<float>(width);
    //         const float ny = static_cast<float>(y) / static_cast<float>(height);
    //
    //         // Base terrain - rolling hills using multiple sine waves
    //         float terrain = 3.0F + 2.0F * std::sin(nx * 3.14159F * 3.0F) * std::cos(ny * 3.14159F * 2.0F);
    //         terrain += 1.5F * std::sin(nx * 6.28F * 2.0F + ny * 6.28F * 1.5F);
    //
    //         // Add some mountains in the upper-left quadrant
    //         if (nx < 0.4F && ny < 0.4F) {
    //             const float distFromCorner = std::sqrt(nx * nx + ny * ny);
    //             terrain += 4.0F * std::exp(-distFromCorner * 5.0F);
    //         }
    //
    //         // Create a valley/river channel running diagonally
    //         const float riverDist = std::abs((nx - ny) * std::sqrt(2.0F));
    //         if (riverDist < 0.15F) {
    //             // River valley - lower terrain
    //             terrain -= 3.0F * (1.0F - riverDist / 0.15F);
    //             terrain = std::max(0.5F, terrain);
    //         }
    //
    //         // Create a lake in the lower-right area
    //         const float lakeCenterX = 0.7F;
    //         const float lakeCenterY = 0.7F;
    //         const float distFromLake = std::sqrt((nx - lakeCenterX) * (nx - lakeCenterX) +
    //                                               (ny - lakeCenterY) * (ny - lakeCenterY));
    //         if (distFromLake < 0.2F) {
    //             // Lake depression
    //             terrain -= 2.5F * (1.0F - distFromLake / 0.2F);
    //             terrain = std::max(0.3F, terrain);
    //         }
    //
    //         // Ensure terrain is not negative
    //         terrain = std::max(0.0F, terrain);
    //
    //         // Add water based on terrain height
    //         float waterDepth = 0.0F;
    //
    //         // River water
    //         if (riverDist < 0.08F && terrain < 2.5F) {
    //             waterDepth = 1.5F - (terrain - 0.5F) * 0.5F;
    //             waterDepth = std::max(0.0F, std::min(2.0F, waterDepth));
    //         }
    //
    //         // Lake water - deeper in center
    //         if (distFromLake < 0.15F) {
    //             waterDepth = 3.0F * (1.0F - distFromLake / 0.15F);
    //             waterDepth = std::max(0.0F, std::min(3.5F, waterDepth));
    //         }
    //
    //         setCell(x, y, Cell{terrain, waterDepth,  false, (riverDist < 0.15F), false, 5.0F});
    //     }
    // }
    updateHeightTexture();
}

void Grid::createShaders() {
    shaderProgram = new QOpenGLShaderProgram();

    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Simulation/Grid/shaders/grid.vert");
    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Simulation/Grid/shaders/grid.frag");
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

            // tex coords
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
            const int topLeft = y * (meshResolution + 1) + x;
            const int topRight = topLeft + 1;
            const int bottomLeft = (y + 1) * (meshResolution + 1) + x;
            const int bottomRight = bottomLeft + 1;

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
    if (heightTexture != nullptr) {
        heightTexture->destroy();
        delete heightTexture;
    }

    heightTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    heightTexture->create();
    heightTexture->setSize(width, height);
    heightTexture->setFormat(QOpenGLTexture::RGB32F);
    heightTexture->allocateStorage();

    heightTexture->setMinificationFilter(QOpenGLTexture::Nearest);
    heightTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
    heightTexture->setWrapMode(QOpenGLTexture::ClampToEdge);

    updateHeightTexture();
}

void Grid::updateHeightTexture() const {
    if (heightTexture == nullptr) {
        qDebug() << "heightTexture is null";
        return;
    }

    // Extract height values from Cell objects into a flat array
    // Format: RGB32F where R = obstacle flag, G = terrain height, B = water depth
    // TODO: if possible optimize to avoid allocation each time
    std::vector<float> textureData(width * height * 3);
    for (unsigned int i = 0; i < heightMap.size(); i++) {
        textureData[i * 3 + 0] = heightMap[i].getType() == OBSTACLE ? 1.0F : 0.0F;
        textureData[i * 3 + 1] = heightMap[i].getTerrainHeight();
        textureData[i * 3 + 2] = heightMap[i].getWaterDepth();
    }

    heightTexture->bind();

    // Set pixel alignment for proper texture upload on macOS
    glContext->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glContext->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, textureData.data());
    glContext->glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // Restore default

    heightTexture->release();
}

void Grid::render(const QMatrix4x4& projection, const QMatrix4x4& view) const {
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
    shaderProgram->setUniformValue("cellSize", cellSize);

    VAO->bind();
    glContext->glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    VAO->release();

    shaderProgram->release();
}

auto Grid::isValidPosition(int x, int y) const -> bool { return x >= 0 && x < width && y >= 0 && y < height; }

auto Grid::getCell(int x, int y) -> Cell*{
    if (!isValidPosition(x, y)) {
        return nullptr;
    }

    return &heightMap[y * width + x];
}

void Grid::setCell(const int x, const int y, const Cell& value) {
    if (!isValidPosition(x, y)) {
        return;
    }

    heightMap[y * width + x] = value;
}

auto Grid::worldPosToGrid(const QVector2D& worldPos) const -> QVector2D {
    float gridX = worldPos.x() / cellSize;
    float gridY = worldPos.y() / cellSize;

    return {gridX, gridY};
}

void Grid::saveHeightmap(const QString& filename) const {
    QFile file(filename);
    // add extension
    if (!filename.endsWith(".map")) {
        file.setFileName(filename + ".map");
    }
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open file for writing:" << filename;
        return;
    }

    QDataStream stream(&file);
    stream << width << height;

    for (Cell cell : heightMap) {
        stream << cell;
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
    int file_width;
    int file_height;
    stream >> file_width >> file_height;
    const bool needResize = file_width != width || file_height != height;
    if (needResize) {
        width = file_width;
        height = file_height;
        heightMap.resize(width * height);
        if (heightTexture != nullptr) {
            delete heightTexture;
            heightTexture = nullptr;
        }
    }

    for (auto &cell : heightMap) {
        stream >> cell;
    }

    if (needResize) {
        createHeightTexture();
    } else {
        updateHeightTexture();
    }
    file.close();
}

void Grid::clearHeightmap(const Cell& defaultCell) {
    std::ranges::fill(heightMap, defaultCell);
    updateHeightTexture();
}

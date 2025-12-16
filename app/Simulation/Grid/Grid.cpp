#include "Grid.h"

#include <QFile>

Grid::Grid(const int width, const int height, const float cellSize, const float maxDepth)
    : glContext(nullptr),
      shaderProgram(nullptr),
      vertexBuffer(nullptr),
      indexBuffer(nullptr),
      VAO(nullptr),
      heightTexture(nullptr),
      pbo{nullptr, nullptr},
      currentPboIndex(0),
      pboSize(0),
      width(width),
      height(height),
      cellSize(cellSize),
      maxDepth(maxDepth),
      indexCount(0),
      meshResolution(200) { // TODO: replace with parameter or something that isnt magic number
    heightMap.resize(width * height, Cell());
}

Grid::~Grid() {
    destroyPBOs();

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
    createPBOs();
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

void Grid::createPBOs() {
    pboSize = static_cast<size_t>(width) * height * 3 * sizeof(float); // 3 values per cell

    // 2 pbos for double buffering
    for (auto & i : pbo) {
        i = new QOpenGLBuffer(QOpenGLBuffer::PixelUnpackBuffer);
        i->create();
        i->bind();
        i->setUsagePattern(QOpenGLBuffer::StreamDraw);
        i->allocate(static_cast<int>(pboSize));
        i->release();
    }

    currentPboIndex = 0;
}

void Grid::destroyPBOs() {
    for (int i = 0; i < 2; i++) {
        if (pbo[i] != nullptr) {
            pbo[i]->destroy();
            delete pbo[i];
            pbo[i] = nullptr;
        }
    }
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

void Grid::updateHeightTexture(){
    if (heightTexture == nullptr) {
        qDebug() << "heightTexture is null";
        return;
    }
    if (pbo[0] == nullptr) {
        qDebug() << "PBOs not created";
        return;
    }


    // double buffering: use one PBO for upload while filling the other
    const int nextPboIndex = (currentPboIndex + 1) % 2;

    heightTexture->bind();
    glContext->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // bind PBO that was filled in previous frame and upload to texture
    pbo[currentPboIndex]->bind();
    glContext->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                                width, height,
                                GL_RGB, GL_FLOAT, nullptr); // nullptr = read from PBO
    pbo[currentPboIndex]->release();

    // bind next PBO and map it for writing
    pbo[nextPboIndex]->bind();
    pbo[nextPboIndex]->allocate(static_cast<int>(pboSize));
    auto* ptr = static_cast<float*>(pbo[nextPboIndex]->map(QOpenGLBuffer::WriteOnly));

    if (ptr != nullptr) {
        for (unsigned int i = 0; i < heightMap.size(); i++) {
            ptr[i * 3 + 0] = heightMap[i].getType() == OBSTACLE ? 1.0F : 0.0F;
            ptr[i * 3 + 1] = heightMap[i].getTerrainHeight();
            ptr[i * 3 + 2] = heightMap[i].getWaterDepth();
        }
        pbo[nextPboIndex]->unmap();
    }

    pbo[nextPboIndex]->release();
    glContext->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    heightTexture->release();

   // swap buffers for next frame
    currentPboIndex = nextPboIndex;
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

void Grid::rollbackToSize(unsigned int originalWidth,unsigned int originalHeight, bool recreateTexture) {
    width = originalWidth;
    height = originalHeight;
    heightMap.resize(width * height);
    std::ranges::fill(heightMap, Cell());

    if (recreateTexture) {
        createPBOs();
        createHeightTexture();
    }
}

auto Grid::loadHeightmap(const QString& filename) -> bool {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open file for reading:" << filename;
        return false;
    }

    // store original state for rollback on failure
    const unsigned int originalWidth = width;
    const unsigned int originalHeight = height;
    bool textureWasDestroyed = false;

    try {
        QDataStream stream(&file);

        int file_width = 0;
        int file_height = 0;
        stream >> file_width >> file_height;

        // validate dimensions
        if (file_width <= 0 || file_height <= 0 ||
            file_width > 10000 || file_height > 10000) {
            file.close();
            qDebug() << "Invalid file dimensions:" << file_width << "x" << file_height;
            return false;
        }

        const bool needResize = file_width != static_cast<int>(width) ||
                                file_height != static_cast<int>(height);
        if (needResize) {
            width = file_width;
            height = file_height;
            heightMap.resize(width * height);
            destroyPBOs();
            if (heightTexture != nullptr) {
                heightTexture->destroy();
                delete heightTexture;
                heightTexture = nullptr;
                textureWasDestroyed = true;
            }
        }

        for (auto &cell : heightMap) {
            if (stream.atEnd()) {
                qDebug() << "Unexpected end of file while reading cells";
                file.close();
                rollbackToSize(originalWidth, originalHeight, true);
                return false;
            }
            stream >> cell;

            if (stream.status() != QDataStream::Ok) {
                qDebug() << "Error reading cell data from file";
                file.close();
                rollbackToSize(originalWidth, originalHeight, true);
                return false;
            }
        }

        if (needResize) {
            createPBOs();
            createHeightTexture();
        } else {
            updateHeightTexture();
        }
        file.close();
        return true;

    } catch (const std::exception& e) {
        qDebug() << "Exception while loading heightmap:" << e.what();
        file.close();
        rollbackToSize(originalWidth, originalHeight, textureWasDestroyed);
        return false;
    } catch (...) {
        qDebug() << "Unknown exception while loading heightmap";
        file.close();
        rollbackToSize(originalWidth, originalHeight, textureWasDestroyed);
        return false;
    }
}

void Grid::clearHeightmap(const Cell& defaultCell) {
    std::ranges::fill(heightMap, defaultCell);
    updateHeightTexture();
}

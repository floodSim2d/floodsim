#include "GLRenderer.h"
#include "../Simulation/Grid.h"
#include <QDebug>
#include <vector>

GLRenderer::GLRenderer(std::shared_ptr<Grid> grid, QWidget *parent)
    : QOpenGLWidget(parent), m_grid(std::move(grid))
{
}

GLRenderer::~GLRenderer()
{
    makeCurrent();
    m_quadVBO.destroy();
    m_quadVAO.destroy();
    glDeleteTextures(1, &m_terrainTextureID);
    glDeleteTextures(1, &m_waterTextureID);
    glDeleteTextures(1, &m_attributesTextureID);
    delete m_terrainProgram;
    doneCurrent();
}

void GLRenderer::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f); // Ciemnoniebieskie tło

    qDebug() << "OpenGL Renderer initialized.";
    qDebug() << "Vendor:" << reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    qDebug() << "Renderer:" << reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    qDebug() << "Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));

    setupShaders();
    setupGeometry();
    setupTextures();
}

void GLRenderer::setupShaders()
{
    m_terrainProgram = new QOpenGLShaderProgram(this);

    if (!m_terrainProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/TerrainShader.vert"))
        qDebug() << "Vertex shader compilation error:" << m_terrainProgram->log();
    if (!m_terrainProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/TerrainShader.frag"))
        qDebug() << "Fragment shader compilation error:" << m_terrainProgram->log();
    if (!m_terrainProgram->link())
        qDebug() << "Shader link error:" << m_terrainProgram->log();
}

void GLRenderer::setupGeometry()
{
    float quadVertices[] = {
        // positions   // texture Coords
        -1.0f,  1.0f,  0.0f, 0.0f, 0.0f, // Top-left vertex -> (0,0) tex coord
        -1.0f, -1.0f,  0.0f, 0.0f, 1.0f, // Bottom-left vertex -> (0,1) tex coord
         1.0f, -1.0f,  0.0f, 1.0f, 1.0f, // Bottom-right vertex -> (1,1) tex coord

        -1.0f,  1.0f,  0.0f, 0.0f, 0.0f, // Top-left vertex -> (0,0) tex coord
         1.0f, -1.0f,  0.0f, 1.0f, 1.0f, // Bottom-right vertex -> (1,1) tex coord
         1.0f,  1.0f,  0.0f, 1.0f, 0.0f  // Top-right vertex -> (1,0) tex coord
    };

    m_quadVAO.create();
    m_quadVAO.bind();

    m_quadVBO.create();
    m_quadVBO.bind();
    m_quadVBO.allocate(quadVertices, sizeof(quadVertices));

    // Atrybut pozycji
    m_terrainProgram->enableAttributeArray(0);
    m_terrainProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 5 * sizeof(float));

    // Atrybut współrzędnych tekstury
    m_terrainProgram->enableAttributeArray(1);
    m_terrainProgram->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));

    m_quadVAO.release();
    m_quadVBO.release();
}

void GLRenderer::setupTextures()
{
    // Tworzymy tekstury tylko raz, przy pierwszym wywołaniu
    if (m_terrainTextureID == 0) {
        glGenTextures(1, &m_terrainTextureID);
    }
    glBindTexture(GL_TEXTURE_2D, m_terrainTextureID);
    // Ustawienia filtrowania
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Ponowna alokacja pamięci na teksturę z nowym rozmiarem
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_grid->getWidth(), m_grid->getHeight(), 0, GL_RED, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Tekstura wody
    if (m_waterTextureID == 0) {
        glGenTextures(1, &m_waterTextureID);
    }
    glBindTexture(GL_TEXTURE_2D, m_waterTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Ponowna alokacja pamięci na teksturę z nowym rozmiarem
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_grid->getWidth(), m_grid->getHeight(), 0, GL_RED, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Tekstura atrybutów (przeszkody, rzeki, etc.)
    if (m_attributesTextureID == 0) {
        glGenTextures(1, &m_attributesTextureID);
    }
    glBindTexture(GL_TEXTURE_2D, m_attributesTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Użyjemy RGBA, żeby mieć miejsce na więcej flag w przyszłości
    // R = obstacle, G = river, B = waterSource
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, m_grid->getWidth(), m_grid->getHeight(), 0, GL_RGB, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::handleGridResize() {
    if (m_grid->getWidth() != m_gridWidth || m_grid->getHeight() != m_gridHeight) {
        qDebug() << "Grid resized, re-creating textures.";
        m_gridWidth = m_grid->getWidth();
        m_gridHeight = m_grid->getHeight();

        // Zamiast usuwać, po prostu ponownie skonfigurujemy tekstury z nowym rozmiarem
        setupTextures();
    }
}

void GLRenderer::updateTerrainTexture()
{
    // Przygotowanie bufora z danymi wysokości
    std::vector<float> heightData;
    heightData.reserve(m_grid->getWidth() * m_grid->getHeight());
    for (int y = 0; y < m_grid->getHeight(); ++y) {
        for (int x = 0; x < m_grid->getWidth(); ++x) {
            heightData.push_back(static_cast<float>(m_grid->getCell(x, y).height));
        }
    }

    glBindTexture(GL_TEXTURE_2D, m_terrainTextureID);
    // Wysłanie zaktualizowanych danych do GPU
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_grid->getWidth(), m_grid->getHeight(), GL_RED, GL_FLOAT, heightData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::updateWaterTexture()
{
    // Przygotowanie bufora z danymi o głębokości wody
    std::vector<float> waterData;
    waterData.reserve(m_grid->getWidth() * m_grid->getHeight());
    for (int y = 0; y < m_grid->getHeight(); ++y) {
        for (int x = 0; x < m_grid->getWidth(); ++x) {
            // Renderer teraz tylko odczytuje dane, niczego nie modyfikuje.
            waterData.push_back(m_grid->getCell(x, y).water_depth);
        }
    }

    glBindTexture(GL_TEXTURE_2D, m_waterTextureID);
    // Wysłanie zaktualizowanych danych do GPU
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_grid->getWidth(), m_grid->getHeight(), GL_RED, GL_FLOAT, waterData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::updateAttributesTexture()
{
    // Przygotowanie bufora z danymi o atrybutach
    std::vector<float> attributesData;
    // 3 kanały (R, G, B) na piksel
    attributesData.reserve(m_grid->getWidth() * m_grid->getHeight() * 3);
    for (int y = 0; y < m_grid->getHeight(); ++y) {
        for (int x = 0; x < m_grid->getWidth(); ++x) {
            const auto& cell = m_grid->getCell(x, y);
            // Kanał R: przeszkoda (0.0 lub 1.0)
            attributesData.push_back(cell.obstacle ? 1.0f : 0.0f);
            // Kanał G: rzeka (0.0 lub 1.0)
            attributesData.push_back(cell.river ? 1.0f : 0.0f);
            // Kanał B: źródło wody (0.0 lub 1.0)
            attributesData.push_back(cell.waterSource ? 1.0f : 0.0f);
        }
    }

    glBindTexture(GL_TEXTURE_2D, m_attributesTextureID);
    // Wysłanie zaktualizowanych danych do GPU
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_grid->getWidth(), m_grid->getHeight(), GL_RGB, GL_FLOAT, attributesData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Sprawdź, czy siatka zmieniła rozmiar i w razie potrzeby zaktualizuj tekstury
    handleGridResize();

    updateTerrainTexture();
    updateWaterTexture();
    updateAttributesTexture();

    m_terrainProgram->bind();
    m_quadVAO.bind();

    // Tekstura terenu na jednostce 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_terrainTextureID);
    m_terrainProgram->setUniformValue("terrainHeightmap", 0);

    // Tekstura wody na jednostce 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_waterTextureID);
    m_terrainProgram->setUniformValue("waterDepthmap", 1);

    // Tekstura atrybutów na jednostce 2
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_attributesTextureID);
    m_terrainProgram->setUniformValue("attributesMap", 2);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_quadVAO.release();
    m_terrainProgram->release();
}

void GLRenderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
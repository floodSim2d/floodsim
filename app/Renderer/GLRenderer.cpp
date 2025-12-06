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
        -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         1.0f, -1.0f,  0.0f, 1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         1.0f, -1.0f,  0.0f, 1.0f, 0.0f,
         1.0f,  1.0f,  0.0f, 1.0f, 1.0f
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
    glGenTextures(1, &m_terrainTextureID);
    glBindTexture(GL_TEXTURE_2D, m_terrainTextureID);
    // Ustawienia filtrowania
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Alokacja pamięci na teksturę
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_grid->getWidth(), m_grid->getHeight(), 0, GL_RED, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
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

void GLRenderer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateTerrainTexture();

    m_terrainProgram->bind();
    m_quadVAO.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_terrainTextureID);
    m_terrainProgram->setUniformValue("terrainHeightmap", 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_quadVAO.release();
    m_terrainProgram->release();
}

void GLRenderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
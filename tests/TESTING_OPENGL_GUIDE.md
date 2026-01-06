# Testowanie Komponentów OpenGL w Qt

## Wprowadzenie

Testowanie komponentów OpenGL jest trudniejsze niż zwykłych komponentów Qt, ponieważ wymaga kontekstu OpenGL. Oto różne podejścia:

## Podejście 1: Offscreen Rendering (Zalecane)

```cpp
#include <QtTest/QtTest>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

class TestOpenGLRenderer : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Create offscreen surface for OpenGL context
        surface = new QOffscreenSurface();
        surface->create();
        
        // Create OpenGL context
        context = new QOpenGLContext();
        context->create();
        context->makeCurrent(surface);
    }

    void cleanupTestCase() {
        context->doneCurrent();
        delete context;
        delete surface;
    }

    void testOpenGLInitialization() {
        QVERIFY(context->isValid());
        
        QOpenGLFunctions* gl = context->functions();
        QVERIFY(gl != nullptr);
        
        // Test OpenGL version
        QString version = QString::fromLatin1((const char*)gl->glGetString(GL_VERSION));
        QVERIFY(!version.isEmpty());
    }

    void testRendererCreation() {
        // Create grid
        Grid* grid = new Grid(10, 10, 1.0f, 50.0f);
        
        // Initialize with offscreen context
        grid->initialize(context->functions());
        
        // Test that resources were created
        QVERIFY(grid->getWidth() == 10);
        QVERIFY(grid->getHeight() == 10);
        
        // Cleanup
        grid->cleanup();
        delete grid;
    }

private:
    QOffscreenSurface* surface;
    QOpenGLContext* context;
};

QTEST_MAIN(TestOpenGLRenderer)
#include "test_opengl_renderer_qt.moc"
```

## Podejście 2: Testowanie Logiki Bez OpenGL

Zamiast testować pełny rendering, testuj logikę oddzielnie:

```cpp
class TestGridLogic : public QObject {
    Q_OBJECT

private slots:
    void testCellOperations() {
        Grid grid(10, 10, 1.0f);
        
        // Test cell access
        Cell* cell = grid.getCell(5, 5);
        QVERIFY(cell != nullptr);
        
        // Test cell modification
        cell->setTerrainHeight(100.0f);
        QCOMPARE(cell->getTerrainHeight(), 100.0f);
        
        // Test boundary checking
        QVERIFY(grid.isValidPosition(5, 5));
        QVERIFY(!grid.isValidPosition(-1, 5));
        QVERIFY(!grid.isValidPosition(5, 100));
    }

    void testGridResize() {
        Grid grid(10, 10, 1.0f);
        
        // Add some data
        Cell* cell = grid.getCell(5, 5);
        cell->setTerrainHeight(50.0f);
        
        // Test that data persists
        QCOMPARE(grid.getCell(5, 5)->getTerrainHeight(), 50.0f);
    }
};
```

## Podejście 3: Mock OpenGL Context

Dla zaawansowanych testów, możesz mockować funkcje OpenGL:

```cpp
class MockOpenGLFunctions : public QOpenGLFunctions {
public:
    // Override OpenGL functions for testing
    void glGenBuffers(GLsizei n, GLuint* buffers) override {
        // Fake implementation for testing
        for (int i = 0; i < n; i++) {
            buffers[i] = fakeBufferId++;
        }
    }
    
    // ... other mocked functions
    
private:
    GLuint fakeBufferId = 1;
};
```

## Podejście 4: Integration Tests z QOpenGLWidget

```cpp
class TestOpenGLWidget : public QObject {
    Q_OBJECT

private slots:
    void testWidgetCreation() {
        Grid* grid = new Grid(10, 10, 1.0f);
        OpenGLRenderer* renderer = new OpenGLRenderer(grid, nullptr);
        
        // Show widget (this creates OpenGL context)
        renderer->show();
        QTest::qWait(100); // Wait for initialization
        
        // Test that OpenGL was initialized
        QVERIFY(renderer->context() != nullptr);
        QVERIFY(renderer->context()->isValid());
        
        // Cleanup
        delete renderer;
        delete grid;
    }
};
```

## Podejście 5: Testowanie Shaderów

```cpp
class TestShaders : public QObject {
    Q_OBJECT

private slots:
    void testShaderCompilation() {
        // Create offscreen context
        QOffscreenSurface surface;
        surface.create();
        
        QOpenGLContext context;
        context.create();
        context.makeCurrent(&surface);
        
        // Test shader compilation
        QOpenGLShaderProgram program;
        
        // Load vertex shader
        QString vertexShaderSource = R"(
            #version 410 core
            layout(location = 0) in vec2 position;
            void main() {
                gl_Position = vec4(position, 0.0, 1.0);
            }
        )";
        
        bool vertexOk = program.addShaderFromSourceCode(
            QOpenGLShader::Vertex, 
            vertexShaderSource
        );
        
        QVERIFY2(vertexOk, program.log().toStdString().c_str());
        
        // Load fragment shader
        QString fragmentShaderSource = R"(
            #version 410 core
            out vec4 fragColor;
            void main() {
                fragColor = vec4(1.0, 0.0, 0.0, 1.0);
            }
        )";
        
        bool fragmentOk = program.addShaderFromSourceCode(
            QOpenGLShader::Fragment,
            fragmentShaderSource
        );
        
        QVERIFY2(fragmentOk, program.log().toStdString().c_str());
        
        // Link program
        bool linked = program.link();
        QVERIFY2(linked, program.log().toStdString().c_str());
        
        context.doneCurrent();
    }
};
```

## Podejście 6: Testowanie Bez GUI (Headless)

Dla CI/CD, możesz użyć Xvfb (Linux) lub podobnych narzędzi:

```bash
# Linux
xvfb-run ./tests_qt

# macOS - nie potrzeba, domyślnie wspiera offscreen
./tests_qt

# Windows - użyj software rendering
set QT_OPENGL=software
tests_qt.exe
```

## Przykład Kompletnego Testu dla OpenGLRenderer

```cpp
#include <QtTest/QtTest>
#include <QOffscreenSurface>
#include <QOpenGLContext>

#include "Renderer/OpenGLRenderer.h"
#include "Simulation/Grid/Grid.h"

class TestOpenGLRendererIntegration : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup offscreen rendering
        surface = new QOffscreenSurface();
        surface->create();
        
        context = new QOpenGLContext();
        context->create();
        QVERIFY(context->isValid());
    }

    void cleanupTestCase() {
        delete context;
        delete surface;
    }

    void testRendererWithOffscreenContext() {
        context->makeCurrent(surface);
        
        // Create grid and renderer
        Grid* grid = new Grid(20, 20, 1.0f);
        
        // Initialize grid with OpenGL context
        grid->initialize(context->functions());
        
        // Set some test data
        Cell* cell = grid->getCell(10, 10);
        cell->setTerrainHeight(50.0f);
        cell->setWaterDepth(10.0f);
        
        // Update texture
        grid->updateHeightTexture();
        
        // Test that grid has data
        QCOMPARE(grid->getCell(10, 10)->getTerrainHeight(), 50.0f);
        
        // Cleanup
        grid->cleanup();
        delete grid;
        
        context->doneCurrent();
    }

    void testCameraOperations() {
        Grid* grid = new Grid(20, 20, 1.0f);
        OpenGLRenderer* renderer = new OpenGLRenderer(grid, nullptr);
        
        // Test camera reset
        renderer->resetCamera();
        
        // Test zoom
        renderer->setZoom(50.0f);
        
        // Test pan (these shouldn't crash)
        renderer->panCamera(10.0f, 10.0f);
        
        // Cleanup
        delete renderer;
        delete grid;
    }

private:
    QOffscreenSurface* surface;
    QOpenGLContext* context;
};

QTEST_MAIN(TestOpenGLRendererIntegration)
#include "test_opengl_integration_qt.moc"
```

## Best Practices

### 1. Oddziel Logikę od Renderingu
- Logikę biznesową (Grid, Cell) testuj bez OpenGL
- Rendering testuj osobno z offscreen context

### 2. Używaj Mock Objects
- Mockuj QOpenGLFunctions dla unit testów
- Używaj prawdziwego OpenGL dla integration testów

### 3. CI/CD Setup
```yaml
# .github/workflows/tests.yml
- name: Install Xvfb (for headless OpenGL)
  run: sudo apt-get install -y xvfb

- name: Run tests
  run: xvfb-run -a ./build/tests/tests_qt
```

### 4. Testuj Shadery Osobno
- Ładuj shadery z plików
- Testuj kompilację i linkowanie
- Używaj prostych test shaderów

### 5. Snapshot Testing
```cpp
void testRendering() {
    // Render do QImage
    QImage result = renderer->grabFramebuffer();
    
    // Porównaj z referencyjnym obrazem
    QImage reference("expected_output.png");
    QCOMPARE(result, reference);
}
```

## Struktura Testów dla Twojego Projektu

```
tests/
├── test_grid.cpp               # Logika Grid (bez OpenGL)
├── test_cell.cpp               # Logika Cell
├── test_flowmodel.cpp          # Symulacja przepływu
├── test_toolpanel_qt.cpp       # UI: ToolPanel
├── test_parameterpanel_qt.cpp  # UI: ParameterPanel
├── test_opengl_offscreen_qt.cpp # OpenGL z offscreen context
├── test_opengl_shaders_qt.cpp   # Kompilacja shaderów
└── test_integration_qt.cpp      # Testy integracyjne
```

## Uruchamianie Testów

```bash
# Build tests
cmake --build build --target tests_qt

# Run all tests
cd build/tests
ctest --verbose

# Run specific test
./test_toolpanel_qt

# Run with Qt Test output
./test_toolpanel_qt -v2
```

## Debugging Testów

```cpp
// Enable Qt debug output
qSetMessagePattern("[%{type}] %{function}: %{message}");

// Add test data
void testSomething_data() {
    QTest::addColumn<int>("input");
    QTest::addColumn<int>("expected");
    
    QTest::newRow("case1") << 1 << 2;
    QTest::newRow("case2") << 2 << 4;
}

void testSomething() {
    QFETCH(int, input);
    QFETCH(int, expected);
    
    QCOMPARE(input * 2, expected);
}
```


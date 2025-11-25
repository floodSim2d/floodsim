add_executable(floodsim
    main.cpp
    UI/MainWindow.h
    UI/MainWindow.cpp
    Simulation/Grid.cpp
    Simulation/Grid.h
    Renderer/OpenGLRenderer.h
    Renderer/OpenGLRenderer.cpp
)

target_link_libraries(floodsim
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::OpenGL
    Qt::OpenGLWidgets
)

# Ensure moc/uic/rcc are active for this target
set_target_properties(floodsim PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

qt_add_resources(floodsim "shaders"
        PREFIX "/"
        FILES
            "Simulation/shaders/grid_vertex.glsl"
            "Simulation/shaders/grid_fragment.glsl"
)
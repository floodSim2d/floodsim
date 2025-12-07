add_executable(floodsim
        main.cpp
        UI/MainWindow.h
        UI/MainWindow.cpp
        Simulation/Grid/Grid.cpp
        Simulation/Grid/Grid.h
        Renderer/OpenGLRenderer.h
        Renderer/OpenGLRenderer.cpp
        Simulation/Grid/Cell.cpp
        Simulation/Grid/Cell.h
)

target_link_libraries(floodsim PRIVATE
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
            "Simulation/Grid/shaders/grid.vert"
            "Simulation/Grid/shaders/grid.frag"
)
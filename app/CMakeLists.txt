add_executable(floodsim
        main.cpp
        UI/MainWindow.h
        UI/MainWindow.cpp
        UI/ToolPanel.h
        UI/ToolPanel.cpp
        UI/ParameterPanel.h
        UI/ParameterPanel.cpp
        UI/SimulationToolbar.h
        UI/SimulationToolbar.cpp
        UI/FileMenuHandler.h
        UI/FileMenuHandler.cpp
        Simulation/Grid/Grid.cpp
        Simulation/Grid/Grid.h
        Renderer/OpenGLRenderer.h
        Renderer/OpenGLRenderer.cpp
        Renderer/WaterRenderer.h
        Renderer/WaterRenderer.cpp
        Simulation/Grid/Cell.cpp
        Simulation/Grid/Cell.h
        Simulation/Tools/PaintTool.h
        Simulation/Tools/PaintTool.cpp
        Simulation/Tools/TerrainGenerator.h
        Simulation/Tools/TerrainGenerator.cpp
        Simulation/FlowModel/FlowModel.h
        Simulation/FlowModel/FlowModel.cpp
        Utils/Logger.h
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
            "Renderer/shaders/water.vert"
            "Renderer/shaders/water.frag"
)
cmake_minimum_required(VERSION 3.16)
project(floodsim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        OpenGLWidgets
)

add_executable(floodsim
        main.cpp
        UI/MainWindow.h
        UI/MainWindow.cpp
        UI/MapView.h
        UI/MapView.cpp
)

target_link_libraries(floodsim PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGLWidgets
)

set_target_properties(floodsim PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
)

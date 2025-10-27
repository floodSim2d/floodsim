cmake_minimum_required(VERSION 4.0)
project(floodsim)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

# Add app and tests subprojects
add_subdirectory(app)
add_subdirectory(tests)

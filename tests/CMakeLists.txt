enable_testing()
include(CTest)
include(FetchContent)

FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)

FetchContent_MakeAvailable(catch2)

FetchContent_GetProperties(catch2)
if(NOT catch2_POPULATED)
  FetchContent_Populate(catch2)
endif()

find_package(Qt6 REQUIRED COMPONENTS Test)

# Catch2 tests / for non-qt components
file(GLOB CATCH2_TEST_SOURCES CONFIGURE_DEPENDS "test_*.cpp")
list(FILTER CATCH2_TEST_SOURCES EXCLUDE REGEX ".*_qt\\.cpp$")

if(CATCH2_TEST_SOURCES)
    add_executable(tests_catch2
        ${CATCH2_TEST_SOURCES}
    )

    target_link_libraries(tests_catch2 PRIVATE Catch2::Catch2WithMain)

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(tests_catch2)
endif()

# qt tests for qt components
file(GLOB QT_TEST_SOURCES CONFIGURE_DEPENDS "*_qt.cpp")

foreach(test_file ${QT_TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)

    add_executable(${test_name}
        ${test_file}
        ${CMAKE_SOURCE_DIR}/app/UI/ToolPanel.cpp
        ${CMAKE_SOURCE_DIR}/app/UI/ParameterPanel.cpp
        ${CMAKE_SOURCE_DIR}/app/UI/SimulationToolbar.cpp
        ${CMAKE_SOURCE_DIR}/app/UI/FileMenuHandler.cpp
        ${CMAKE_SOURCE_DIR}/app/Simulation/Tools/PaintTool.cpp
        ${CMAKE_SOURCE_DIR}/app/Simulation/Grid/Grid.cpp
        ${CMAKE_SOURCE_DIR}/app/Simulation/Grid/Cell.cpp
        ${CMAKE_SOURCE_DIR}/app/Simulation/FlowModel/FlowModel.cpp
        ${CMAKE_SOURCE_DIR}/app/Renderer/OpenGLRenderer.cpp
        ${CMAKE_SOURCE_DIR}/app/Renderer/WaterRenderer.cpp
    )

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/app
    )

    target_link_libraries(${test_name} PRIVATE
        Qt::Test
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGL
        Qt::OpenGLWidgets
    )

    set_target_properties(${test_name} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()


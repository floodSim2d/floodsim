include(FetchContent)

FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0 # pin a reasonably recent version
)

FetchContent_MakeAvailable(catch2)

# Ensure we have access to the source directory to point include to single_include
FetchContent_GetProperties(catch2)
if(NOT catch2_POPULATED)
  # If not populated this call will populate it
  FetchContent_Populate(catch2)
endif()

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_executable(tests
    ${TEST_SOURCES}
)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
# Discover tests in the created test executable
catch_discover_tests(tests)
cmake_minimum_required(VERSION 3.24)
project(hybrid_viewer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build with optimizations by default on macOS and Linux
# Visual Studio picks Release with the config option instead.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Put the C++ runtime inside viewer.exe on Windows
# so the program also runs on computers without the Visual C++ runtime installed.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


# Libraries downloaded at configure time
# Each one is pinned to a release tag so both of us build the same code.
include(FetchContent)

# GLFW makes the window and reads the mouse and keyboard
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.5.1
  GIT_SHALLOW TRUE)

# GLM has the vector and matrix math
FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.3
  GIT_SHALLOW TRUE)

# Dear ImGui draws the control panel
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.93.0
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(glfw glm imgui)


# Dear ImGui has no build file of its own
# so we list its sources here, with the GLFW and OpenGL 3 backends.
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)


# glad loads the OpenGL functions
# The files in third_party were generated for OpenGL 4.1 core.
add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)


# The viewer program
add_executable(viewer
  src/camera.cpp
  src/main.cpp
  src/mesh.cpp
  src/renderer.cpp
  src/shader.cpp
  src/ui.cpp)
target_link_libraries(viewer PRIVATE glad glfw glm::glm imgui)

# The program reads the shaders straight from the source folder
# so it works from any folder and the R key reloads the files we edit.
target_compile_definitions(viewer PRIVATE SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/")

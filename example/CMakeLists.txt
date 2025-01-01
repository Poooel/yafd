CMAKE_MINIMUM_REQUIRED(VERSION 3.21)
PROJECT(yafd_demo)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# external
# =========
# GLFW3: https://github.com/glfw/glfw
ADD_SUBDIRECTORY(external-demo/glfw)
# OpenGL: https://www.opengl.org/
FIND_PACKAGE(OpenGL REQUIRED)
# ImGui: https://github.com/ocornut/imgui
INCLUDE_DIRECTORIES(../external/imgui)
INCLUDE_DIRECTORIES(../external/imgui/backends)
# yafd
INCLUDE_DIRECTORIES(../)

# Source files
# ============
SET(SOURCES main.cpp)
SET(IMGUI_SOURCES ../external/imgui/backends/imgui_impl_glfw.cpp ../external/imgui/backends/imgui_impl_opengl3.cpp ../external/imgui/imgui.cpp ../external/imgui/imgui_draw.cpp ../external/imgui/imgui_tables.cpp ../external/imgui/imgui_widgets.cpp ../external/imgui/misc/cpp/imgui_stdlib.cpp ../external/imgui/imgui_demo.cpp)
SET(YAFD_SOURCES ../yafd.cpp ../yafd_demo.cpp)

# Link
# ====
ADD_EXECUTABLE(yafd_demo ${SOURCES} ${IMGUI_SOURCES} ${YAFD_SOURCES})
TARGET_LINK_LIBRARIES(yafd_demo PRIVATE glfw ${OPENGL_LIBRARIES})

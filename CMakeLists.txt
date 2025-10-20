cmake_minimum_required(VERSION 3.16)
project(RadiationSim LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(ENABLE_GUI "Build Qt GUI" ON)

# OpenMP (optionnel)
find_package(OpenMP)
if (OpenMP_CXX_FOUND)
  add_compile_definitions(USE_OPENMP)
endif()

# GLM via pkg-config (optionnel)
find_package(PkgConfig)
pkg_check_modules(GLM QUIET glm)
if (GLM_FOUND)
  add_compile_definitions(HAVE_GLM)
endif()

# ---- Core library (tous les .cpp du noyau, y compris utils) ----
file(GLOB CORE_SRC
    src/visualization/*.cpp
    src/core/*.cpp
    src/geometry/*.cpp
    src/simulation/*.cpp
    src/utils/*.cpp
)
add_library(RadiationCore STATIC ${CORE_SRC})
target_include_directories(RadiationCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if (OpenMP_CXX_FOUND)
  target_link_libraries(RadiationCore PUBLIC OpenMP::OpenMP_CXX)
endif()

# ---- Console ----
add_executable(RadiationSimConsole src/demo_console.cpp)
target_include_directories(RadiationSimConsole PRIVATE include)
target_link_libraries(RadiationSimConsole PRIVATE RadiationCore)

# ---- GUI (Qt6 Widgets/Gui requis, OpenGLWidgets optionnel) ----
if (ENABLE_GUI)
  find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Gui)
  find_package(Qt6 OPTIONAL_COMPONENTS OpenGLWidgets)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)

  add_executable(RadiationSimGUI
  include/ui/MainWindow.h
  src/ui/MaterialEditor.cpp
  src/ui/GeometryEditor.cpp
  src/ui/SensorEditor.cpp
  src/ui/SourceEditor.cpp
    src/ui/MainWindow.cpp
    src/main.cpp
  )
  target_include_directories(RadiationSimGUI PRIVATE include)
  target_link_libraries(RadiationSimGUI PRIVATE RadiationCore Qt6::Widgets Qt6::Gui)
  if (TARGET Qt6::OpenGLWidgets)
    target_link_libraries(RadiationSimGUI PRIVATE Qt6::OpenGLWidgets)
    target_compile_definitions(RadiationSimGUI PRIVATE HAS_QOPENGLWIDGET=1)
  endif()
endif()

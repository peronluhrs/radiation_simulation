cmake_minimum_required(VERSION 3.18)
project(RadiationSimulation VERSION 1.0.0 LANGUAGES CXX)

# ------------------ Options ------------------
option(ENABLE_GUI "Build Qt GUI" ON)

# ------------------ Langage/Policies ----------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ------------------ Dépendances génériques ----
find_package(Threads REQUIRED)
find_package(OpenMP)
# GLM est optionnel : on a glm_simple.h en fallback
find_package(PkgConfig QUIET)
find_package(glm QUIET)

# ------------------ Includes généraux ---------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ------------------ OpenMP flags --------------
if(OpenMP_CXX_FOUND)
  message(STATUS "OpenMP activé")
endif()

# ============================================================
#                      LIBRAIRIE CORE
#  (AUCUNE dépendance Qt ici !)
# ============================================================
file(GLOB_RECURSE CORE_SRC
  src/core/*.cpp
  src/geometry/*.cpp
  src/simulation/*.cpp
  src/utils/*.cpp
)

# Évite que des fichiers UI/visualization se glissent dans le core
list(FILTER CORE_SRC EXCLUDE REGEX ".*/ui/.*")
list(FILTER CORE_SRC EXCLUDE REGEX ".*/visualization/.*")

add_library(RadiationCore STATIC ${CORE_SRC})
target_include_directories(RadiationCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(RadiationCore PRIVATE $<$<BOOL:${glm_FOUND}>:USE_GLM>)
if(OpenMP_CXX_FOUND)
  target_link_libraries(RadiationCore PUBLIC OpenMP::OpenMP_CXX)
  target_compile_definitions(RadiationCore PUBLIC USE_OPENMP)
endif()
target_link_libraries(RadiationCore PUBLIC Threads::Threads)

# ============================================================
#                   EXE CONSOLE (toujours)
# ============================================================
add_executable(RadiationSimConsole
  src/demo_console.cpp
)
target_link_libraries(RadiationSimConsole PRIVATE RadiationCore)
target_compile_definitions(RadiationSimConsole PRIVATE CONSOLE_VERSION)

# ============================================================
#                        GUI Qt (option)
# ============================================================
if(ENABLE_GUI)
  # On essaie Qt6 Widgets/Gui/OpenGL ; si introuvable on coupe la GUI
  set(QT_NEEDED_COMPONENTS Widgets Gui OpenGL)
  find_package(Qt6 6.2 QUIET COMPONENTS ${QT_NEEDED_COMPONENTS})
  if(NOT Qt6_FOUND)
    message(WARNING "Qt6 non trouvé, désactivation de la GUI. (Installe: qt6-base-dev)")
    set(ENABLE_GUI OFF)
  endif()
endif()

if(ENABLE_GUI)
  message(STATUS "Qt6 trouvé - Interface graphique activée")
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTORCC ON)

  # Sources GUI — seulement ici les fichiers Qt/OpenGL
  set(GUI_SRC
    src/main.cpp
    src/ui/MainWindow.cpp
    include/ui/MainWindow.h
    src/visualization/View3D.cpp
    include/visualization/View3D.h
  )

  add_executable(RadiationSimGUI ${GUI_SRC}
  include/visualization/Renderer.h

  src/visualization/Renderer.cpp

  include/visualization/VtkLegacyLoader.h

  src/visualization/VtkLegacyLoader.cpp

  include/ui/GeometryEditor.h

  include/ui/MaterialEditor.h

  include/ui/SensorEditor.h

  include/ui/SourceEditor.h

  src/ui/GeometryEditor.cpp

  src/ui/MaterialEditor.cpp

  src/ui/SensorEditor.cpp

  src/ui/SourceEditor.cpp
)
  target_include_directories(RadiationSimGUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

  target_link_libraries(RadiationSimGUI
    PRIVATE
      RadiationCore
      Qt6::Widgets
      Qt6::Gui
      Qt6::OpenGL
  )

  # Si tu utilises QOpenGLWidget quelque part, essaye d'avoir le module aussi (optionnel)
  find_package(Qt6 QUIET COMPONENTS OpenGLWidgets)
  if(Qt6OpenGLWidgets_FOUND)
    target_link_libraries(RadiationSimGUI PRIVATE Qt6::OpenGLWidgets)
    target_compile_definitions(RadiationSimGUI PRIVATE HAS_QOPENGLWIDGET=1)
  endif()
endif()

# ============================================================
#                        Messages build
# ============================================================
message(STATUS "Configuration du projet:")
message(STATUS "  - Version: ${PROJECT_VERSION}")
message(STATUS "  - Standard C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - Type de build: ${CMAKE_BUILD_TYPE}")
message(STATUS "  - Interface graphique: ${ENABLE_GUI}")
message(STATUS "  - OpenMP: $<IF:$<BOOL:${OpenMP_CXX_FOUND}>,TRUE,FALSE>")

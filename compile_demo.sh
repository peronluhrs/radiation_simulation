#!/bin/bash

# Script de compilation directe pour la démo console

echo "Compilation de la version console de démonstration..."

# Variables
INCLUDE_DIR="include"
SRC_FILES="src/demo_console.cpp src/core/Material.cpp src/core/Scene.cpp src/core/Sensor.cpp src/core/Source.cpp src/geometry/Object3D.cpp src/geometry/Box.cpp src/simulation/MonteCarloEngine.cpp src/simulation/Particle.cpp src/utils/BVH.cpp src/utils/Math.cpp"
OUTPUT="RadiationSimConsole"
CPPFLAGS="-std=c++20 -O2 -I$INCLUDE_DIR -DCONSOLE_VERSION"

# Recherche de GLM (math library)
if pkg-config --exists glm; then
    echo "GLM trouvé via pkg-config"
    CPPFLAGS="$CPPFLAGS $(pkg-config --cflags glm)"
elif [ -d "/usr/include/glm" ]; then
    echo "GLM trouvé dans /usr/include"
elif [ -d "/usr/local/include/glm" ]; then
    echo "GLM trouvé dans /usr/local/include" 
    CPPFLAGS="$CPPFLAGS -I/usr/local/include"
else
    echo "ATTENTION: GLM non trouvé - utilisation d'une implémentation de base"
    CPPFLAGS="$CPPFLAGS -DUSE_BASIC_MATH"
fi

# OpenMP si disponible
if g++ -fopenmp -x c++ -E /dev/null &>/dev/null; then
    echo "OpenMP disponible"
    CPPFLAGS="$CPPFLAGS -fopenmp -DUSE_OPENMP"
    LDFLAGS="-fopenmp"
else
    echo "OpenMP non disponible"
    LDFLAGS=""
fi

# Compilation
echo "Commande de compilation:"
echo "g++ $CPPFLAGS $SRC_FILES $LDFLAGS -o $OUTPUT"
echo ""

if g++ $CPPFLAGS $SRC_FILES $LDFLAGS -o $OUTPUT; then
    echo "✓ Compilation réussie!"
    echo "Exécutable créé: $OUTPUT"
    echo ""
    echo "Pour tester:"
    echo "  ./$OUTPUT --help"
    echo "  ./$OUTPUT"
else
    echo "✗ Échec de la compilation"
    exit 1
fi
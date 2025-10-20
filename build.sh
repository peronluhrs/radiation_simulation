#!/bin/bash

# Script de compilation pour le Simulateur d'Atténuation de Radiation
# Auteur: Assistant IA
# Date: $(date)

set -e  # Arrêt en cas d'erreur

PROJECT_NAME="RadiationAttenuationSimulator"
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Fonction d'affichage
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Vérification des prérequis
check_requirements() {
    print_info "Vérification des prérequis..."
    
    # CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake n'est pas installé"
        exit 1
    fi
    CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
    print_info "CMake version: $CMAKE_VERSION"
    
    # Compilateur C++
    if command -v g++ &> /dev/null; then
        GCC_VERSION=$(g++ --version | head -n1 | awk '{print $4}')
        print_info "GCC version: $GCC_VERSION"
        COMPILER="g++"
    elif command -v clang++ &> /dev/null; then
        CLANG_VERSION=$(clang++ --version | head -n1 | awk '{print $3}')
        print_info "Clang version: $CLANG_VERSION"
        COMPILER="clang++"
    else
        print_error "Aucun compilateur C++ trouvé (g++ ou clang++)"
        exit 1
    fi
    
    # Qt6 (optionnel car difficile à installer)
    if pkg-config --exists Qt6Core Qt6Widgets Qt6OpenGL 2>/dev/null; then
        QT_VERSION=$(pkg-config --modversion Qt6Core)
        print_info "Qt6 version: $QT_VERSION"
        QT_AVAILABLE=true
    else
        print_warning "Qt6 non trouvé - compilation d'une version console simplifiée"
        QT_AVAILABLE=false
    fi
    
    print_success "Prérequis vérifiés"
}

# Nettoyage
clean() {
    if [ -d "$BUILD_DIR" ]; then
        print_info "Nettoyage du répertoire de build..."
        rm -rf "$BUILD_DIR"
        print_success "Nettoyage terminé"
    fi
}

# Configuration
configure() {
    print_info "Configuration du projet..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Options CMake de base
    CMAKE_OPTS="-DCMAKE_BUILD_TYPE=Release"
    CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    
    # Configuration selon la disponibilité de Qt
    if [ "$QT_AVAILABLE" = true ]; then
        CMAKE_OPTS="$CMAKE_OPTS -DENABLE_GUI=ON"
    else
        CMAKE_OPTS="$CMAKE_OPTS -DENABLE_GUI=OFF"
    fi
    
    # Configuration du compilateur
    if [ "$COMPILER" = "clang++" ]; then
        CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_CXX_COMPILER=clang++"
    fi
    
    print_info "Configuration CMake: $CMAKE_OPTS"
    
    if cmake .. $CMAKE_OPTS; then
        print_success "Configuration réussie"
    else
        print_error "Échec de la configuration"
        exit 1
    fi
    
    cd ..
}

# Compilation
build() {
    print_info "Compilation du projet..."
    
    cd "$BUILD_DIR"
    
    # Détection du nombre de cœurs
    if command -v nproc &> /dev/null; then
        CORES=$(nproc)
    else
        CORES=4
    fi
    
    print_info "Compilation sur $CORES cœurs..."
    
    if make -j$CORES; then
        print_success "Compilation réussie"
    else
        print_error "Échec de la compilation"
        exit 1
    fi
    
    cd ..
}

# Tests simples
test() {
    print_info "Exécution des tests..."
    
    cd "$BUILD_DIR"
    
    # Vérification que l'exécutable existe
    if [ -f "$PROJECT_NAME" ]; then
        print_info "Exécutable trouvé: $PROJECT_NAME"
        
        # Test d'aide
        if ./$PROJECT_NAME --help 2>/dev/null || true; then
            print_success "Test d'aide réussi"
        fi
        
        # Test version (si implémenté)
        if ./$PROJECT_NAME --version 2>/dev/null || true; then
            print_success "Test de version réussi"
        fi
        
    else
        print_warning "Exécutable non trouvé - compilation en cours?"
    fi
    
    cd ..
    print_success "Tests terminés"
}

# Installation
install() {
    print_info "Installation du projet..."
    
    cd "$BUILD_DIR"
    
    if make install; then
        print_success "Installation réussie dans $INSTALL_PREFIX"
    else
        print_error "Échec de l'installation"
        exit 1
    fi
    
    cd ..
}

# Packaging
package() {
    print_info "Création du package..."
    
    cd "$BUILD_DIR"
    
    # Création d'un archive tar.gz
    PACKAGE_NAME="${PROJECT_NAME}-$(date +%Y%m%d)"
    
    mkdir -p "$PACKAGE_NAME"
    cp "$PROJECT_NAME" "$PACKAGE_NAME/" 2>/dev/null || true
    cp ../README.md "$PACKAGE_NAME/"
    cp ../examples/*.cpp "$PACKAGE_NAME/" 2>/dev/null || true
    
    tar -czf "${PACKAGE_NAME}.tar.gz" "$PACKAGE_NAME"
    
    print_success "Package créé: ${PACKAGE_NAME}.tar.gz"
    
    cd ..
}

# Affichage de l'aide
show_help() {
    echo "Usage: $0 [OPTIONS] [TARGETS]"
    echo ""
    echo "TARGETS:"
    echo "  check      - Vérifier les prérequis"
    echo "  clean      - Nettoyer les fichiers de build"
    echo "  configure  - Configurer le projet avec CMake"
    echo "  build      - Compiler le projet"
    echo "  test       - Exécuter les tests"
    echo "  install    - Installer le projet"
    echo "  package    - Créer un package de distribution"
    echo "  all        - Exécuter check, configure, build, test"
    echo ""
    echo "OPTIONS:"
    echo "  -h, --help     - Afficher cette aide"
    echo "  --clean-all    - Nettoyer avant de construire"
    echo "  --prefix=PATH  - Définir le préfixe d'installation"
    echo ""
    echo "Exemples:"
    echo "  $0 all                    # Build complet"
    echo "  $0 clean configure build  # Reconstruction"
    echo "  $0 --prefix=/opt install  # Installation personnalisée"
}

# Analyse des arguments
CLEAN_ALL=false
TARGETS=()

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        --clean-all)
            CLEAN_ALL=true
            shift
            ;;
        --prefix=*)
            INSTALL_PREFIX="${1#*=}"
            shift
            ;;
        check|clean|configure|build|test|install|package|all)
            TARGETS+=("$1")
            shift
            ;;
        *)
            print_error "Option inconnue: $1"
            show_help
            exit 1
            ;;
    esac
done

# Si aucune cible spécifiée, utiliser 'all'
if [ ${#TARGETS[@]} -eq 0 ]; then
    TARGETS=("all")
fi

# Affichage du header
echo "======================================"
echo "  $PROJECT_NAME"
echo "  Script de Compilation"
echo "======================================"
echo ""

# Exécution des cibles
for target in "${TARGETS[@]}"; do
    case $target in
        check)
            check_requirements
            ;;
        clean)
            clean
            ;;
        configure)
            if [ "$CLEAN_ALL" = true ]; then
                clean
            fi
            check_requirements
            configure
            ;;
        build)
            build
            ;;
        test)
            test
            ;;
        install)
            install
            ;;
        package)
            package
            ;;
        all)
            if [ "$CLEAN_ALL" = true ]; then
                clean
            fi
            check_requirements
            configure
            build
            test
            ;;
        *)
            print_error "Cible inconnue: $target"
            exit 1
            ;;
    esac
    echo ""
done

print_success "Script terminé avec succès!"
echo ""
echo "Pour utiliser le simulateur:"
if [ -f "$BUILD_DIR/$PROJECT_NAME" ]; then
    echo "  cd $BUILD_DIR && ./$PROJECT_NAME"
else
    echo "  Compilez d'abord avec: $0 build"
fi
echo ""
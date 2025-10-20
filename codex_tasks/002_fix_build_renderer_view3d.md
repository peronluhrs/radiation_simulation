# Task: T1.1 – Fix build Renderer/View3D (no features)

## Goal
Compiler/lier sur Ubuntu 24.04 + Qt6. Si incertain: stubs.

## Règles
- QOpenGLFunctions uniquement.
- Pas de VAO, pas de glPolygonMode, pas de fixed pipeline.
- Conserver signatures:
  initialize(QWidget*/QWindow*), resize, beginFrame, endFrame, renderOnce,
  drawGrid(float,float,float), drawAxes(float).

## Étapes
1) Implémentations possibles no-op (sauf beginFrame: clear + depth).
2) ./scripts/dev_build.sh
3) Commit: fix(gui): compile-only guard for Renderer/View3D (GLES-safe, drawing stubs)

## Acceptation
- build OK, GUI se linke et se lance.

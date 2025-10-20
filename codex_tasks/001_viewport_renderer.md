# Task 001 — Stabiliser et structurer le viewport 3D

## Objectif
Rendre le viewport stable, avec un rendu minimal propre (axes, grille, caméra orbit) et brancher un pipeline de dessin scène/sources/capteurs non-bloquant.

## Contexte actuel
- GUI lancé via `RadiationSimGUI`, **viewport** = `QOpenGLWindow` dans un `QWidget::createWindowContainer(...)`.
- `Renderer` est un en-tête "header-only" minimal (stub), `View3D` fait le clear + axes en GL immédiat.
- `GeometryEditor`, `MaterialEditor`, `SensorEditor`, `SourceEditor` existent en stubs.

## À faire (MVP)
1. **Camera orbit**: rotation (drag gauche), pan (drag milieu), zoom (molette), reset (double-clic).
2. **Grid** XY et axes XYZ (VAO simple, pas d’Immediate Mode si possible).
3. **Renderer**: 
   - fournir une API stateless : `beginFrame(viewProj)`, draw helpers (lines, boxes), `endFrame()`.
   - ne pas dépendre de `QOpenGLWidget`; utiliser le contexte courant de `QOpenGLWindow`.
4. **Scene preview**:
   - dessiner bbox des `Object3D` (wireframe), gizmos pour sensors/sources.
   - drapeau "wireframe" toggle depuis les actions déjà présentes.
5. **Thread-safety**:
   - aucune capture d’OpenGL depuis les threads de simulation; uniquement le thread GUI.

## Contraintes
- **Ne casse pas** `RadiationSimConsole`.
- **CMake**: pas de Qt dans `RadiationCore`. Tout le Qt reste dans l’exe GUI.
- **OpenMP**: inchangé.
- **GLM**: optionnel (fallback `glm_simple.h`).

## Acceptation
- Build `scripts/dev_build.sh` OK (console + GUI).
- GUI: viewport stable, axes+grille visibles, navigation fluide.
- Pas de crash pendant une simulation (même si le rendu reste minimal).

## Points bonus (si temps)
- Frustum culling simple.
- Mode “solid” vs “wireframe”.

## Repères
- Viewport: `include/visualization/View3D.h`, `src/visualization/View3D.cpp`
- Renderer: `include/visualization/Renderer.h` (header-only)
- MainWindow: `include/ui/MainWindow.h`, `src/ui/MainWindow.cpp`


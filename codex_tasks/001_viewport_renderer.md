# Task: Stabiliser le viewport et rendre le Renderer compatible GLES (grid + axes)

## Contexte
Nous avons un GUI Qt6 qui se lance, un `View3D` basé sur `QOpenGLWindow` et un `Renderer` minimal.
Objectifs d'après les docs:
- Vue 3D temps réel robuste, indépendante du moteur (archi MVC).  
- Rendu de base: grille, axes, helpers de scène (prépare T2).  
(Refs: Software Design Document §§1–3; Definition §§2.7, 2.1)  

## Objectif
1) Assurer un rendu stable **sans** `glPolygonMode` ni VAO (compat GLES via `QOpenGLFunctions` uniquement).  
2) Implémenter **drawGrid(size, step, minorAlpha)** et **drawAxes(length)** en GL_LINES.  
3) Exposer API côté `Renderer` déjà appelée par `View3D` (initialize/resizer/beginFrame/endFrame/renderOnce).  
4) Conserver build CLI + GUI via `./scripts/dev_build.sh`.

## Contraintes techniques
- Utiliser **uniquement** `QOpenGLContext::currentContext()->functions()` (pas `extraFunctions()`).
- Pas de VAO/VBO obligatoires pour l’instant: on accepte du **mode immédiat** (glBegin/glEnd) si dispo; sinon, fournir un chemin “client-side arrays” simple.  
- Ne pas réintroduire `glPolygonMode`, `glGenVertexArrays`, etc.  
- Garder la compat Qt6 sur Linux (Ubuntu 24.04), GCC 13.

## Fichiers à éditer
- `include/visualization/Renderer.h`
- `src/visualization/Renderer.cpp`
- `src/visualization/View3D.cpp` (uniquement si ajustement mineur d’appels)
- (optionnel) `src/ui/MainWindow.cpp` si nécessité de ne **pas** casser l’initialisation actuelle.

## Détails d’implémentation

### A) Renderer.h
- Confirmer l’API suivante (créer/compléter si manquante) :
  ```cpp
  class Renderer {
  public:
    Renderer(); ~Renderer();

    void initialize(QWidget* w);    // pour QOpenGLWidget
    void initialize(QWindow* win);  // pour QOpenGLWindow (View3D)
    void setViewport(QWidget* w);   // no-op si QOpenGLWindow
    void attachScene(std::shared_ptr<Scene> s);

    void resize(int w, int h);
    void beginFrame(const glm::mat4& viewProj);
    void endFrame();
    void renderOnce();

    void setWireframe(bool on); bool wireframe() const;

    // dessinateurs de debug
    void drawGrid(float size, float step, float minorAlpha);
    void drawAxes(float length);

  private:
    void ensureInitialized();
    std::shared_ptr<Scene> m_scene{};
    QWidget* m_viewport{nullptr};
    QOpenGLFunctions* m_gl{nullptr};
    bool m_wireframe{false};
  };

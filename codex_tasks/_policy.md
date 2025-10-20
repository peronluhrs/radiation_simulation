# Policy – Build must stay green

- Ubuntu 24.04, Qt6, GCC13, GLES-compatible.
- GL via QOpenGLFunctions uniquement.
- Interdits: glPolygonMode, VAO, glBegin/glEnd, client-state arrays.
- Si doute: stub (no-op) pour compiler.

## Étapes obligatoires
1) ./scripts/dev_build.sh (si non-zéro: corriger, ne pas commit)
2) Coller ~100 lignes du log d’échec.
3) Commits verts et atomiques.

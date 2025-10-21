#!/usr/bin/env python3
import sys, math

def cube_vertices(L, W, H):
    x = L/2.0; y = W/2.0; z = H/2.0
    # indexation:
    # 0: (-x,-y,-z), 1:(x,-y,-z), 2:(x,y,-z), 3:(-x,y,-z)
    # 4: (-x,-y, z), 5:(x,-y, z), 6:(x,y, z), 7:(-x,y, z)
    return [
        (-x,-y,-z), ( x,-y,-z), ( x, y,-z), (-x, y,-z),
        (-x,-y, z), ( x,-y, z), ( x, y, z), (-x, y, z),
    ]

# Triangles d'un cube (orientation "vers l'extérieur" pour ce repère)
TRIS_OUT = [
    (0,1,2), (0,2,3),       # -Z
    (4,5,6), (4,6,7),       # +Z
    (0,4,5), (0,5,1),       # -Y
    (1,5,6), (1,6,2),       # +X
    (2,6,7), (2,7,3),       # +Y
    (3,7,4), (3,4,0),       # -X
]

# Arêtes uniques du cube (12 arêtes)
EDGES = [
    (0,1),(1,2),(2,3),(3,0),  # bas
    (4,5),(5,6),(6,7),(7,4),  # haut
    (0,4),(1,5),(2,6),(3,7),  # montants
]

def write_vtk_polydata(path, points, tris, edges=None, title="Thick box"):
    with open(path, "w") as f:
        f.write("# vtk DataFile Version 3.0\n")
        f.write(f"{title}\n")
        f.write("ASCII\n")
        f.write("DATASET POLYDATA\n")
        f.write(f"POINTS {len(points)} float\n")
        for p in points:
            f.write(f"{p[0]} {p[1]} {p[2]}\n")

        f.write(f"POLYGONS {len(tris)} {len(tris)*4}\n")
        for a,b,c in tris:
            f.write(f"3 {a} {b} {c}\n")

        if edges:
            f.write(f"LINES {len(edges)} {len(edges)*3}\n")
            for i,j in edges:
                f.write(f"2 {i} {j}\n")

def main():
    # Args: out.vtk L W H thickness
    out = sys.argv[1] if len(sys.argv) > 1 else "examples/box_thick.vtk"
    L   = float(sys.argv[2]) if len(sys.argv) > 2 else 2.0
    W   = float(sys.argv[3]) if len(sys.argv) > 3 else 2.0
    H   = float(sys.argv[4]) if len(sys.argv) > 4 else 2.0
    t   = float(sys.argv[5]) if len(sys.argv) > 5 else 0.2

    Li = L - 2*t; Wi = W - 2*t; Hi = H - 2*t
    if Li <= 0 or Wi <= 0 or Hi <= 0:
        print("Erreur: l'épaisseur est trop grande (dimensions internes négatives).", file=sys.stderr)
        sys.exit(2)

    outer = cube_vertices(L, W, H)      # 0..7
    inner = cube_vertices(Li, Wi, Hi)   # 0..7 (on remappera indices après concat)

    # Concat points: outer[0..7], inner[0..7] => indices 0..15
    pts = outer + inner

    # Triangles: 12 pour outer (orientation sortante),
    #            12 pour inner mais orientation INVERSÉE (normales vers l'intérieur de la cavité).
    outer_tris = [(a,b,c) for (a,b,c) in TRIS_OUT]
    inner_tris = []
    for (a,b,c) in TRIS_OUT:
        # mêmes sommets mais sur le bloc "inner" (+8), ordre inversé
        inner_tris.append( (c+8, b+8, a+8) )

    tris = outer_tris + inner_tris

    # Edges (pour le rendu filaire simple): outer + inner
    edges = EDGES + [(i+8, j+8) for (i,j) in EDGES]

    write_vtk_polydata(out, pts, tris, edges, title=f"Thick box L={L} W={W} H={H} t={t}")
    print(f"[OK] Écrit: {out}")

if __name__ == "__main__":
    main()

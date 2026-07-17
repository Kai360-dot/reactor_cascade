"""Cascade estimate: how many unit-2 physics evals would an ANN-first
(rho <= 0) gate have skipped? Replays mlp_cu.txt over the logged inlets
of the unit-2 NSFEAS run."""

import csv

import numpy as np


def load_mlp(path):  # mlp_ffvar.hpp text format
    tok = open(path).read().split()
    L, i, layers = int(tok[0]), 1, []
    for _ in range(L):
        nin, nout = int(tok[i]), int(tok[i + 1])
        i += 2
        W = np.array(tok[i : i + nin * nout], float).reshape(nout, nin)
        i += nin * nout
        b = np.array(tok[i : i + nout], float)
        i += nout
        layers.append((W, b))
    return layers


def forward(layers, x):  # affine + tanh, linear last layer
    h = x
    for k, (W, b) in enumerate(layers):
        h = h @ W.T + b
        if k + 1 < len(layers):
            h = np.tanh(h)
    return h


cu = load_mlp("data/mlp_cu.txt")
total = skipped = 0
for name in ("unit2_live", "unit2_dead", "unit2_discard"):
    rows = np.array(
        [
            [float(v) for v in r]
            for r in csv.reader(open(f"data/{name}.csv"))
            if r and r[0] != "crit"
        ]
    )
    rho = forward(cu, rows[:, 4:7])[:, 0]  # inlet cols cA,cB,cC
    n, nskip = len(rows), int((rho > 0).sum())
    total += n
    skipped += nskip
    print(f"{name:14s} {n:7d} pts   rho>0: {nskip:7d} ({100 * nskip / n:5.1f}%)")
phys = total - skipped
print(
    f"\ntotal evals {total}, gate-rejected {skipped} ({100 * skipped / total:.1f}%)"
    f"\nphysics evals remaining under cascade: {phys}"
    f"  (= {phys / 2:.0f} flowsheet-equivalents, vs {total / 2:.0f} without)"
)

"""
This script trains 3 separate ANN surrogates on the simulation data from cstr-1
- c1: Feasibility predictor (if output <= 0 <=> feasible)
- s1: Input Output Map (T1, tau1 => c1A, c1B, c1C)
- cu: Feasibility predictor (similar to c1) but using outputs of cstr-1 as
  inputs, i.e. (c1A, c1B, c1C => In/Feasible (>/<= 0))

mlp_predcheck.csv is used for checking whether the predictions of the ANN are
robustly mirrored in the C++ executables, i.e. to check for conversion errors.
"""
import csv

import numpy as np
import torch
import torch.nn as nn

torch.manual_seed(0)
rng = np.random.default_rng(0)

MAX_PER_FILE = 20_000
EPOCHS = 3000
HID = 16


def load(fname):
    with open(fname) as f:
        rows = np.array(
            [[float(v) for v in r] for r in csv.reader(f) if r and r[0] != "crit"]
        )
        if len(rows) > MAX_PER_FILE:
            rows = rows[rng.choice(len(rows), MAX_PER_FILE, replace=False)]
        return rows


data = np.vstack([load(f"unit1_{k}.csv") for k in ("live", "dead", "discard")])
# [crit, feasprob, T1, tau1, cA1, cB1, cC1]
crit, d1, outlet = data[:, 0:1], data[:, 2:4], data[:, 4:7]
print(f"training rows: {len(data)} feasible: {(crit < 0).mean():.1%}")

# heldout split for C++ cross check and metrics
idx = rng.permutation(len(data))
n_hold = 64
hold, tr = idx[:n_hold], idx[n_hold:]


def mlp(nin, nout):
    return nn.Sequential(
        nn.Linear(nin, HID),
        nn.Tanh(),
        nn.Linear(HID, HID),
        nn.Tanh(),
        nn.Linear(HID, nout),
    )


def standardize(a):
    mu, sig = a.mean(0), a.std(0) + 1e-12
    return (a - mu) / sig, mu, sig


def train(name, x, y):
    xs, xmu, xsig = standardize(x[tr])
    ys, ymu, ysig = standardize(y[tr])
    net = mlp(x.shape[1], y.shape[1])
    opt = torch.optim.Adam(net.parameters(), lr=1e-3)
    tx, ty = (
        torch.tensor(xs, dtype=torch.float64),
        torch.tensor(ys, dtype=torch.float64),
    )
    net.double()
    for ep in range(EPOCHS):
        opt.zero_grad()
        loss = nn.functional.mse_loss(net(tx), ty)
        loss.backward()
        opt.step()
        if ep % 500 == 0:
            print(f" {name} epoch {ep:5d} mse(std) {loss.item():.3e}")
    return net, (xmu, xsig, ymu, ysig)


def fold_and_export(name, net, scales):
    """Fold input/output standardization into first/last affine layers so the
    C++ side applies raw weights only, then write mlp_ffvar.hpp's format."""
    xmu, xsig, ymu, ysig = scales
    lins = [m for m in net if isinstance(m, nn.Linear)]
    W = [m.weight.detach().numpy().copy() for m in lins]
    b = [m.bias.detach().numpy().copy() for m in lins]
    b[0] = b[0] - W[0] @ (xmu / xsig)  # x_std = (x - xmu)/xsig
    W[0] = W[0] / xsig[None, :]
    W[-1] = ysig[:, None] * W[-1]  # y = y_std*ysig + ymu
    b[-1] = ysig * b[-1] + ymu
    with open(f"mlp_{name}.txt", "w") as f:
        f.write(f"{len(W)}\n")
        for Wl, bl in zip(W, b):
            f.write(f"{Wl.shape[1]} {Wl.shape[0]}\n")
            for row in Wl:
                f.write(" ".join(f"{v:.17g}" for v in row) + "\n")
            f.write(" ".join(f"{v:.17g}" for v in bl) + "\n")
    return W, b


def predict(W, b, x):
    h = x
    for i, (Wl, bl) in enumerate(zip(W, b)):
        h = h @ Wl.T + bl
        if i + 1 < len(W):
            h = np.tanh(h)
    return h


nets = {}
for name, x, y in (("c1", d1, crit), ("s1", d1, outlet), ("cu", outlet, crit)):
    net, scales = train(name, x, y)
    nets[name] = fold_and_export(name, net, scales)

# ---- held-out metrics ------------------------------------------------------
p_c1 = predict(*nets["c1"], d1[hold])  # pyright:ignore
p_s1 = predict(*nets["s1"], d1[hold])  # pyright:ignore
p_cu = predict(*nets["cu"], outlet[hold])  # pyright:ignore
lab = crit[hold] <= 0
print(
    f"c1 held-out: rmse {np.sqrt(np.mean((p_c1 - crit[hold]) ** 2)):.4f}  "
    f"class acc {((p_c1 <= 0) == lab).mean():.1%}  "
    f"false-feas {((p_c1 <= 0) & ~lab).mean():.1%}"
)
print(f"s1 held-out: rmse per outlet {np.sqrt(np.mean((p_s1 - outlet[hold]) ** 2, 0))}")
print(f"cu held-out: class acc {((p_cu <= 0) == lab).mean():.1%}")

# ---- cross-check file for reference/mlp_check.cpp --------------------------
with open("mlp_predcheck.csv", "w") as f:
    f.write("T1,tau1,cA1,cB1,cC1,c1_pred,sA_pred,sB_pred,sC_pred,cu_pred\n")
    for i, k in enumerate(hold):
        f.write(
            ",".join(
                f"{v:.17g}"
                for v in (*d1[k], *outlet[k], p_c1[i, 0], *p_s1[i], p_cu[i, 0])
            )
            + "\n"
        )
print("wrote mlp_c1.txt mlp_s1.txt mlp_cu.txt mlp_predcheck.csv")

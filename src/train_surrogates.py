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

"""
Visual test for trellis_plot.

Constructs 3x3 = 9 synthetic bins over col_var, row_var in [0, 3] and puts
two points into each bin (blue set: bin center; red set: center + 0.25).
The x/y coordinates are set EQUAL to col_var/row_var, so the plot is
correct exactly if:

  * every panel contains exactly 2 points (1 blue + 1 red), and
  * the points appear at the position in the panel that MIRRORS the
    panel's own position in the grid: left column -> points on the left,
    right column -> points on the right, TOP row -> points at the TOP
    (rows bin high -> low, top -> bottom).

So the dots should march diagonally: top-right panel has dots in its own
top-right area, bottom-left panel in its bottom-left area, etc. Any point
showing up in the wrong panel, or the pattern being mirrored/flipped,
means the binning or row ordering is broken.

Run:  uv run test_trellis.py   -> writes trellis_test.png.
"""

import numpy as np

from trellis import trellis_plot

NBINS = 3
LO, HI = 0.0, 3.0
centers = (np.arange(NBINS) + 0.5) * (HI - LO) / NBINS  # 0.5, 1.5, 2.5

# one point per (row_bin, col_bin) combination, for each of the two sets
cc, rr = np.meshgrid(centers, centers)
cc, rr = cc.ravel(), rr.ravel()

# blue set: exactly at the bin centers
blue_col, blue_row = cc, rr
# red set: offset by +0.25 (still inside the same bin, width = 1.0)
red_col, red_row = cc + 0.25, rr + 0.25

fig = trellis_plot(
    # x equals col_var, y equals row_var -> position mirrors the grid
    [blue_col, red_col],
    [blue_row, red_row],
    [blue_col, red_col],
    [blue_row, red_row],
    colors=["royalblue", "crimson"],
    labels=["center", "center + 0.25"],
    x_label="x (= col_var)",
    y_label="y (= row_var)",
    col_label="col_var",
    row_label="row_var",
    ncols=NBINS,
    nrows=NBINS,
    x_lim=(LO, HI),
    y_lim=(LO, HI),
    col_lim=(LO, HI),
    row_lim=(LO, HI),
    title="trellis test: each panel = 2 dots, mirroring the panel's grid position",
)

# ----------------------------------------------------------------------
# annotate every point with its full 4-D coordinates (x, y, col, row)
# so the viewer can check each dot against the panel it landed in
# ----------------------------------------------------------------------
panels = fig.axes[: NBINS * NBINS]  # subplots are created first, row-major
row_centers_top_down = centers[::-1]  # grid rows bin high -> low
mid = (LO + HI) / 2

for r in range(NBINS):
    for c in range(NBINS):
        ax = panels[r * NBINS + c]
        for offset, color in ((0.0, "royalblue"), (0.25, "crimson")):
            pc = centers[c] + offset
            pr = row_centers_top_down[r] + offset
            px, py = pc, pr  # x = col_var, y = row_var by construction
            ha = "left" if px < mid else "right"  # keep text inside the panel
            dx = 6 if ha == "left" else -6
            ax.annotate(
                f"({px:.2f}, {py:.2f}, {pc:.2f}, {pr:.2f})",
                xy=(px, py),
                xytext=(dx, 0),
                textcoords="offset points",
                fontsize=6,
                color=color,
                ha=ha,
                va="center",
            )

fig.text(
    0.02,
    0.975,
    "point labels: (x, y, col_var, row_var)",
    ha="left",
    fontsize=8,
    color="0.3",
)
fig.savefig("trellis_test.png", dpi=150)
print("wrote trellis_test.png")

print()
print("expected: every panel count = 2 (printed matrix above must be all 2s);")
print("dots sit at the panel's own grid position (top row -> dots on top, ...);")
print("each dot's printed (x, y, col_var, row_var) matches its panel's bins")

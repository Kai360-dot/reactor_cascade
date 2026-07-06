"""
General trellis-plot template for visualizing 4-D point clouds.

Give it four equal-length 1-D arrays (x, y, col_var, row_var) whose i-th
entries together form one point in 4-D space. The figure is a grid of
(x, y) scatter panels:

  * grid COLUMNS bin `col_var`  (low -> high, left -> right)
  * grid ROWS    bin `row_var`  (high -> low, top -> bottom)

Each panel carries small range bars (top: col_var, right: row_var) showing
which bin of the binned variables it represents.
Usage:

    from trellis import trellis_plot
    fig = trellis_plot(T1, tau1, T2, tau2,
                       x_label="$T_1$ [K]", y_label=r"$\tau_1$ [min]",
                       col_label="$T_2$ [K]", row_label=r"$\tau_2$ [min]",
                       save="trellis.png")

Optionally pass `color_by=` (an array, e.g. a robustness margin) to color
points by magnitude with a single-hue colormap instead of uniform blue.
"""

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import MaxNLocator

__all__ = ["trellis_plot"]


def _limits(arr, lim):
    """Resolve axis limits: given (lo, hi) or None -> data range with tiny pad."""
    if lim is not None:
        return float(lim[0]), float(lim[1])
    lo, hi = float(np.min(arr)), float(np.max(arr))
    if lo == hi:  # degenerate: all values equal
        lo, hi = lo - 0.5, hi + 0.5
    return lo, hi


def _in_bin(vals, lo, hi, top):
    """Half-open [lo, hi); the top-most bin also includes its upper edge,
    so every in-range point lands in exactly one bin."""
    if top:
        return (vals >= lo) & (vals <= hi)
    return (vals >= lo) & (vals < hi)


def trellis_plot(
    x,
    y,
    col_var,
    row_var,
    *,
    x_label="x",
    y_label="y",
    col_label="col",
    row_label="row",
    ncols=3,
    nrows=3,
    x_lim=None,
    y_lim=None,
    col_lim=None,
    row_lim=None,
    color_by=None,
    color_label="",
    title=None,
    point_size=14,
    save=None,
    dpi=150,
    verbose=True,
):
    """4-D trellis: (x, y) scatter panels, columns bin `col_var`
    (low->high), rows bin `row_var` (high->low).

    Parameters
    ----------
    x, y, col_var, row_var : 1-D array-like, same length
        The four coordinates; entry i of each array is one 4-D point.
    x_label, y_label, col_label, row_label : str
        Axis / range-bar labels (LaTeX ok, e.g. r"$\\tau_1$ [min]").
    ncols, nrows : int
        Panel grid size; also the number of bins for col_var / row_var.
    x_lim, y_lim, col_lim, row_lim : (lo, hi) or None
        Ranges; default to each array's data range. Points outside
        col_lim/row_lim fall in no panel (a warning is printed).
    color_by : 1-D array-like or None
        If given, color points by this value (single-hue 'viridis');
        otherwise uniform blue like the reference figure.
    color_label : str
        Colorbar label when color_by is used.
    title : str or None
        Figure title.
    save : str or None
        If given, save the figure to this path.

    Returns
    -------
    fig : matplotlib Figure
    """
    x = np.asarray(x, float).ravel()
    y = np.asarray(y, float).ravel()
    cv = np.asarray(col_var, float).ravel()
    rv = np.asarray(row_var, float).ravel()
    if not (len(x) == len(y) == len(cv) == len(rv)):
        raise ValueError(
            f"arrays must have equal length, got {len(x)}, {len(y)}, {len(cv)}, {len(rv)}"
        )
    c = None if color_by is None else np.asarray(color_by, float).ravel()
    if c is not None and len(c) != len(x):
        raise ValueError("color_by must have the same length as the coordinate arrays")

    xlo, xhi = _limits(x, x_lim)
    ylo, yhi = _limits(y, y_lim)
    clo_, chi_ = _limits(cv, col_lim)
    rlo_, rhi_ = _limits(rv, row_lim)

    col_edges = np.linspace(clo_, chi_, ncols + 1)
    row_edges = np.linspace(rlo_, rhi_, nrows + 1)
    col_bins = list(zip(col_edges[:-1], col_edges[1:]))  # left  = lowest col bin
    row_bins = list(zip(row_edges[:-1], row_edges[1:]))[::-1]  # top   = highest row bin

    fig, axes = plt.subplots(
        nrows, ncols, figsize=(3.4 * ncols, 3.4 * nrows), squeeze=False
    )

    vmin = vmax = None
    if c is not None:
        vmin, vmax = float(c.min()), float(c.max())
    sc_last = None
    counts = np.zeros((nrows, ncols), dtype=int)

    for r, (rlo, rhi) in enumerate(row_bins):
        row_mask = _in_bin(rv, rlo, rhi, top=(r == 0))
        for ci, (clo, chi) in enumerate(col_bins):
            ax = axes[r][ci]
            m = row_mask & _in_bin(cv, clo, chi, top=(ci == ncols - 1))
            counts[r, ci] = int(m.sum())

            if c is not None:
                sc_last = ax.scatter(
                    x[m],
                    y[m],
                    c=c[m],
                    s=point_size,
                    cmap="viridis",
                    vmin=vmin,
                    vmax=vmax,
                    edgecolor="navy",
                    linewidth=0.2,
                    alpha=0.85,
                )
            else:
                ax.scatter(
                    x[m],
                    y[m],
                    s=point_size,
                    facecolor="royalblue",
                    edgecolor="navy",
                    linewidth=0.3,
                    alpha=0.6,
                )

            ax.set_xlim(xlo, xhi)
            ax.set_ylim(ylo, yhi)
            ax.xaxis.set_major_locator(MaxNLocator(4))
            ax.yaxis.set_major_locator(MaxNLocator(4))
            ax.tick_params(labelsize=8)
            if r == nrows - 1:
                ax.set_xlabel(x_label, fontsize=10)
            if ci == 0:
                ax.set_ylabel(y_label, fontsize=10)

            # top range bar: col_var, this column's bin highlighted
            tb = ax.inset_axes([0.0, 1.07, 1.0, 0.09])
            tb.set_xlim(clo_, chi_)
            tb.set_ylim(0, 1)
            tb.axvspan(clo, chi, color="0.55")
            tb.set_yticks([])
            tb.set_xticks([clo_, chi_], [f"{clo_:.4g}", f"{chi_:.4g}"])
            for tl, ha in zip(tb.get_xticklabels(), ("left", "right")):
                tl.set_ha(ha)
            tb.tick_params(labelsize=7, length=2, pad=1)
            tb.set_title(col_label, fontsize=8.5, pad=2)

            # right range bar: row_var, this row's bin highlighted
            rb = ax.inset_axes([1.06, 0.0, 0.08, 1.0])
            rb.set_ylim(rlo_, rhi_)
            rb.set_xlim(0, 1)
            rb.axhspan(rlo, rhi, color="0.55")
            rb.set_xticks([])
            rb.yaxis.set_label_position("right")
            rb.yaxis.tick_right()
            rb.set_yticks([rlo_, rhi_], [f"{rlo_:.4g}", f"{rhi_:.4g}"])
            for tl, va in zip(rb.get_yticklabels(), ("bottom", "top")):
                tl.set_va(va)
            rb.tick_params(labelsize=7, length=2, pad=1)
            rb.set_ylabel(row_label, fontsize=8.5, rotation=270, labelpad=11)

    fig.subplots_adjust(
        left=0.075,
        right=0.90,
        bottom=0.085,
        top=0.88 if title else 0.92,
        wspace=0.62,
        hspace=0.88,
    )
    if title:
        fig.suptitle(title, fontsize=14, y=0.975)
    fig.text(
        0.5,
        0.015,
        f"columns bin {col_label} (low to high);  rows bin {row_label} (high to low)",
        ha="center",
        fontsize=8,
        color="0.45",
    )

    if c is not None and sc_last is not None:
        cax = fig.add_axes([0.935, 0.20, 0.014, 0.5])  # pyright: ignore
        fig.colorbar(sc_last, cax=cax, label=color_label)

    n_shown = int(counts.sum())
    if verbose:
        print(
            f"panel counts (rows = {row_label} high->low, cols = {col_label} low->high):"
        )
        print(counts)
        print(f"sum of panels = {n_shown}   total points = {len(x)}")
    if n_shown < len(x):
        print(
            f"WARNING: {len(x) - n_shown} points fall outside col_lim/row_lim "
            "and are not shown in any panel"
        )

    if save:
        fig.savefig(save, dpi=dpi)
        if verbose:
            print(f"wrote {save}")
    return fig


# ----------------------------------------------------------------------
# Demo: plot the nested-sampling live points (T1, tau1, T2, tau2)
# ----------------------------------------------------------------------
if __name__ == "__main__":
    import pandas as pd

    df = pd.read_csv("../live_points.csv")
    trellis_plot(
        df["T1"],
        df["tau1"],
        df["T2"],
        df["tau2"],
        x_label="$T_1$ [K]",
        y_label=r"$\tau_1$ [min]",
        col_label="$T_2$ [K]",
        row_label=r"$\tau_2$ [min]",
        title="Feasible operating region (live points)",
        save="trellis_live.png",
    )

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

Multiple point sets (e.g. dead + live points): pass a LIST of arrays for
each of the four coordinates, one entry per set, plus colors/labels.
Sets are drawn in order, so put the one that should sit on top last:

    fig = trellis_plot([dead.T1, live.T1], [dead.tau1, live.tau1],
                       [dead.T2, live.T2], [dead.tau2, live.tau2],
                       colors=["crimson", "royalblue"],
                       labels=["dead", "live"], ...)

Optionally pass `color_by=` (an array, e.g. a robustness margin; single
point set only) to color points by magnitude with a colormap instead.
"""

import matplotlib.colors as mcolors
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.lines import Line2D
from matplotlib.ticker import MaxNLocator

__all__ = ["trellis_plot"]

# fixed set order: first set, second set, ... (never cycled)
DEFAULT_COLORS = ["royalblue", "crimson", "seagreen", "darkorange", "mediumpurple"]


def _as_sets(a):
    """Normalize one coordinate argument to a list of 1-D float arrays.
    A list/tuple whose elements are themselves array-like means multiple sets."""
    if isinstance(a, (list, tuple)) and len(a) > 0 and np.ndim(a[0]) >= 1:
        return [np.asarray(s, float).ravel() for s in a]
    return [np.asarray(a, float).ravel()]


def _limits(arrs, lim):
    """Resolve axis limits: given (lo, hi) or None -> combined data range."""
    if lim is not None:
        return float(lim[0]), float(lim[1])
    lo = min(float(np.min(a)) for a in arrs)
    hi = max(float(np.max(a)) for a in arrs)
    if lo == hi:  # degenerate: all values equal
        lo, hi = lo - 0.5, hi + 0.5
    return lo, hi


def _in_bin(vals, lo, hi, top):
    """Half-open [lo, hi); the top-most bin also includes its upper edge,
    so every in-range point lands in exactly one bin."""
    if top:
        return (vals >= lo) & (vals <= hi)
    return (vals >= lo) & (vals < hi)


def _darken(color, factor=0.55):
    return tuple(factor * ch for ch in mcolors.to_rgb(color))


def trellis_plot(
    x,
    y,
    col_var,
    row_var,
    *,
    colors=None,
    labels=None,
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
    x, y, col_var, row_var : 1-D array-like, or list of 1-D array-like
        The four coordinates; entry i of each array is one 4-D point.
        Pass a list of arrays for each argument to overlay several point
        sets (all four lists need the same number of sets, and the arrays
        within one set the same length). Sets are drawn in order, so the
        last set sits on top.
    colors : list of matplotlib colors, optional
        One face color per point set (default: blue, red, green, ...).
    labels : list of str, optional
        One legend label per point set; a legend is added when given.
    x_label, y_label, col_label, row_label : str
        Axis / range-bar labels (LaTeX ok, e.g. r"$\\tau_1$ [min]").
    ncols, nrows : int
        Panel grid size; also the number of bins for col_var / row_var.
    x_lim, y_lim, col_lim, row_lim : (lo, hi) or None
        Ranges; default to the combined data range over all sets. Points
        outside col_lim/row_lim fall in no panel (a warning is printed).
    color_by : 1-D array-like or None
        Single point set only: color points by this value ('viridis');
        otherwise points use the flat set colors.
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
    xs = _as_sets(x)
    ys = _as_sets(y)
    cvs = _as_sets(col_var)
    rvs = _as_sets(row_var)
    nsets = len(xs)
    if not (len(ys) == len(cvs) == len(rvs) == nsets):
        raise ValueError(
            f"x, y, col_var, row_var must have the same number of point sets, "
            f"got {nsets}, {len(ys)}, {len(cvs)}, {len(rvs)}"
        )
    for k in range(nsets):
        if not (len(xs[k]) == len(ys[k]) == len(cvs[k]) == len(rvs[k])):
            raise ValueError(
                f"point set {k}: arrays must have equal length, got "
                f"{len(xs[k])}, {len(ys[k])}, {len(cvs[k])}, {len(rvs[k])}"
            )

    if color_by is not None and nsets > 1:
        raise ValueError("color_by is only supported with a single point set")
    c = None if color_by is None else np.asarray(color_by, float).ravel()
    if c is not None and len(c) != len(xs[0]):
        raise ValueError("color_by must have the same length as the coordinate arrays")

    if colors is None:
        colors = DEFAULT_COLORS[:nsets]
    if len(colors) < nsets:
        raise ValueError(f"need {nsets} colors, got {len(colors)}")
    edge_colors = [_darken(col) for col in colors]
    if labels is not None and len(labels) != nsets:
        raise ValueError(f"need {nsets} labels, got {len(labels)}")

    xlo, xhi = _limits(xs, x_lim)
    ylo, yhi = _limits(ys, y_lim)
    clo_, chi_ = _limits(cvs, col_lim)
    rlo_, rhi_ = _limits(rvs, row_lim)

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

    # leave room on the right for the colorbar when one is drawn
    right_edge = 0.84 if c is not None else 0.90

    for r, (rlo, rhi) in enumerate(row_bins):
        row_masks = [_in_bin(rv, rlo, rhi, top=(r == 0)) for rv in rvs]
        for ci, (clo, chi) in enumerate(col_bins):
            ax = axes[r][ci]
            masks = [
                rm & _in_bin(cv, clo, chi, top=(ci == ncols - 1))
                for rm, cv in zip(row_masks, cvs)
            ]
            counts[r, ci] = int(sum(m.sum() for m in masks))

            for k, m in enumerate(masks):
                if c is not None:
                    sc_last = ax.scatter(
                        xs[k][m],
                        ys[k][m],
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
                        xs[k][m],
                        ys[k][m],
                        s=point_size,
                        facecolor=colors[k],
                        edgecolor=edge_colors[k],
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
        right=right_edge,
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

    if labels is not None and c is None:
        handles = [
            Line2D(
                [],
                [],
                linestyle="",
                marker="o",
                markersize=7,
                markerfacecolor=colors[k],
                markeredgecolor=edge_colors[k],
                alpha=0.8,
                label=labels[k],
            )
            for k in range(nsets)
        ]
        fig.legend(
            handles=handles,
            loc="upper right",
            bbox_to_anchor=(0.99, 0.985),
            frameon=False,
            fontsize=9,
        )

    if c is not None and sc_last is not None:
        cax = fig.add_axes([0.94, 0.20, 0.014, 0.5])  # pyright: ignore
        cb = fig.colorbar(sc_last, cax=cax, label=color_label)
        cb.formatter.set_powerlimits((-2, 3))  # scientific notation for tiny/huge values
        cb.update_ticks()

    n_total = sum(len(a) for a in xs)
    n_shown = int(counts.sum())
    if verbose:
        print(
            f"panel counts (rows = {row_label} high->low, cols = {col_label} low->high):"
        )
        print(counts)
        print(f"sum of panels = {n_shown}   total points = {n_total}")
    if n_shown < n_total:
        print(
            f"WARNING: {n_total - n_shown} points fall outside col_lim/row_lim "
            "and are not shown in any panel"
        )

    if save:
        fig.savefig(save, dpi=dpi)
        if verbose:
            print(f"wrote {save}")
    return fig


# ----------------------------------------------------------------------
# Demo: dead points (red, below) + live points (blue, on top),
# plus a single-set variant colored by the crit margin
# ----------------------------------------------------------------------
if __name__ == "__main__":
    import pandas as pd

    live = pd.read_csv("../data/live_points.csv")
    dead = pd.read_csv("../data/dead_points.csv")
    trellis_plot(
        [dead["T1"], live["T1"]],
        [dead["tau1"], live["tau1"]],
        [dead["T2"], live["T2"]],
        [dead["tau2"], live["tau2"]],
        colors=["crimson", "royalblue"],
        labels=["dead", "live"],
        x_label="$T_1$ [K]",
        y_label=r"$\tau_1$ [min]",
        col_label="$T_2$ [K]",
        row_label=r"$\tau_2$ [min]",
        title="Operating region: live vs dead points",
        save="trellis_live_dead.png",
    )

    trellis_plot(
        live["T1"],
        live["tau1"],
        live["T2"],
        live["tau2"],
        color_by=-live["crit"],
        color_label="robustness margin (-crit)",
        x_label="$T_1$ [K]",
        y_label=r"$\tau_1$ [min]",
        col_label="$T_2$ [K]",
        row_label=r"$\tau_2$ [min]",
        title="Feasible operating region (live points)",
        save="trellis_live_colored.png",
    )

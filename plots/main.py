import pandas as pd

from trellis import trellis_plot

df_live_ = pd.read_csv("../data/live_points.csv")
df_dead_ = pd.read_csv("../data/dead_points.csv")
df_disc_ = pd.read_csv("../data/discard_points.csv")


def plot_df(df_live: pd.DataFrame, df_dead: pd.DataFrame):
    T1_live, tau1_live, T2_live, tau2_live = (
        df_live["T1"],
        df_live["tau1"],
        df_live["T2"],
        df_live["tau2"],
    )
    # crit_live = df_live["crit"]

    T1_dead, tau1_dead, T2_dead, tau2_dead = (
        df_dead["T1"],
        df_dead["tau1"],
        df_dead["T2"],
        df_dead["tau2"],
    )

    trellis_plot(
        [T1_dead, T1_live],
        [T2_dead, T2_live],
        [tau1_dead, tau1_live],
        [tau2_dead, tau2_live],
        x_label="$T_1$ [K]",
        y_label=r"$T_2$ [K]",
        col_label=r"$\tau_1$ [s]",
        row_label=r"$\tau_2$ [s]",
        save="trellis.png",
        colors=["crimson", "royalblue"],
        ncols=3,
        nrows=3,
        labels=["dead", "live"],
        # color_by=crit_live, # only usable on one set (e.g. live points only)
    )


def main():
    plot_df(df_live_, df_dead_)


if __name__ == "__main__":
    main()

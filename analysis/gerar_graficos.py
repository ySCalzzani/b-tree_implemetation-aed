#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# gerar_graficos.py
#
# Gera as figuras da analise experimental usadas nos slides (slides/img/*.png).
# Le os CSVs de data/ com a biblioteca padrao `csv` (sem pandas/seaborn) e
# desenha com matplotlib — assim roda no .venv do projeto, que tem apenas
# matplotlib + numpy, e e regeneravel por `make graficos`.
#
# Fontes de dados:
#   data/resultados_experimentos.csv  (M,numKeys,tipo,reads,writes,avg_reads,
#       avg_writes,tempo_total,tempo_cpu,tempo_io,bytes,altura)
#   data/reuso_comparacao.csv         (..., reuso=com/sem, ..., bytes, ...)
#
# Cada figura responde a uma ou mais perguntas experimentais do enunciado.
# -----------------------------------------------------------------------------
import csv
import os

import matplotlib
matplotlib.use("Agg")  # backend sem display (headless / make)
import matplotlib.pyplot as plt

# Diretorios (relativos a raiz do projeto, independente de onde rodar).
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "data")
OUT = os.path.join(ROOT, "slides", "img")
os.makedirs(OUT, exist_ok=True)

# Paleta alinhada ao deck (azul do PPTX de referencia).
BLUE = "#4472C4"
ORANGE = "#ED7D31"
GRAY = "#595959"
GREEN = "#70AD47"

plt.rcParams.update({
    "figure.dpi": 150,
    "savefig.dpi": 150,
    "font.size": 12,
    "axes.grid": True,
    "grid.alpha": 0.3,
    "axes.axisbelow": True,
})

N_BIG = 1_000_000  # volume de referencia para varios graficos


def load(path):
    """Le um CSV em lista de dicts com os campos numericos ja convertidos."""
    rows = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            for k in ("M", "numKeys", "reads", "writes", "bytes", "altura"):
                if k in r and r[k] != "":
                    r[k] = int(r[k])
            for k in ("avg_reads", "avg_writes", "tempo_total",
                      "tempo_cpu", "tempo_io"):
                if k in r and r[k] != "":
                    r[k] = float(r[k])
            rows.append(r)
    return rows


def sub(rows, **flt):
    """Filtra linhas por igualdade de campos e ordena por M crescente."""
    out = [r for r in rows if all(r[k] == v for k, v in flt.items())]
    return sorted(out, key=lambda r: r["M"])


def save(fig, name):
    path = os.path.join(OUT, name)
    fig.savefig(path, bbox_inches="tight")
    plt.close(fig)
    print("  ok:", os.path.relpath(path, ROOT))


# -----------------------------------------------------------------------------
def fig_reads_vs_m(res):
    """avg_reads x M (Seq e Rand), N=10^6. Q: nº de acessos; onde o I/O cai."""
    fig, ax = plt.subplots(figsize=(7.2, 4.2))
    for tipo, color, mark in (("Seq", BLUE, "o"), ("Rand", ORANGE, "s")):
        d = sub(res, numKeys=N_BIG, tipo=tipo)
        ax.plot([r["M"] for r in d], [r["avg_reads"] for r in d],
                marker=mark, color=color, lw=2, label=tipo)
    ax.set_xscale("log")
    ax.set_xlabel("Ordem M (escala log)")
    ax.set_ylabel("Leituras medias por operacao")
    ax.set_title("Acessos a disco vs. ordem M  ($N=10^6$)")
    ax.legend(title="Insercao")
    save(fig, "fig_reads_vs_m.png")


def fig_altura(res):
    """altura x M e altura x N. Q: M e N influenciam a altura (~log_M N)."""
    fig, (a1, a2) = plt.subplots(1, 2, figsize=(9.6, 4.0))

    for tipo, color, mark in (("Seq", BLUE, "o"), ("Rand", ORANGE, "s")):
        d = sub(res, numKeys=N_BIG, tipo=tipo)
        a1.plot([r["M"] for r in d], [r["altura"] for r in d],
                marker=mark, color=color, lw=2, label=tipo)
    a1.set_xscale("log")
    a1.set_xlabel("Ordem M (log)")
    a1.set_ylabel("Altura (niveis)")
    a1.set_title("Altura vs. M  ($N=10^6$)")
    a1.legend(title="Insercao")

    for m, color, mark in ((3, BLUE, "o"), (10, GREEN, "^"), (100, ORANGE, "s")):
        d = [r for r in res if r["M"] == m and r["tipo"] == "Seq"]
        d = sorted(d, key=lambda r: r["numKeys"])
        a2.plot([r["numKeys"] for r in d], [r["altura"] for r in d],
                marker=mark, color=color, lw=2, label=f"M={m}")
    a2.set_xscale("log")
    a2.set_xlabel("Nº de chaves N (log)")
    a2.set_ylabel("Altura (niveis)")
    a2.set_title("Altura vs. N  (Seq)")
    a2.legend()

    fig.suptitle("Altura da arvore: cresce com N, encolhe com M", y=1.02)
    save(fig, "fig_altura.png")


def fig_tradeoff(res):
    """tempo total/cpu/io x M (Seq, N=10^6) + zona otima. Q: CPU vs I/O; M otimo."""
    d = sub(res, numKeys=N_BIG, tipo="Seq")
    Ms = [r["M"] for r in d]
    fig, ax = plt.subplots(figsize=(7.2, 4.2))
    ax.axvspan(100, 500, color=GREEN, alpha=0.12, label="zona otima")
    ax.plot(Ms, [r["tempo_total"] for r in d], marker="o", color=GRAY,
            lw=2.5, label="total")
    ax.plot(Ms, [r["tempo_io"] for r in d], marker="s", color=BLUE,
            lw=2, label="I/O (disco)")
    ax.plot(Ms, [r["tempo_cpu"] for r in d], marker="^", color=ORANGE,
            lw=2, label="CPU")
    ax.set_xscale("log")
    ax.set_xlabel("Ordem M (escala log)")
    ax.set_ylabel("Tempo (s)")
    ax.set_title("Trade-off CPU x I/O  ($N=10^6$, Seq)")
    ax.legend()
    save(fig, "fig_tradeoff.png")


def fig_ocupacao(res):
    """bytes (MB) x M, Seq vs Rand, N=10^6. Q: ocupacao; padrao de insercao."""
    fig, ax = plt.subplots(figsize=(7.2, 4.2))
    for tipo, color, mark in (("Seq", BLUE, "o"), ("Rand", ORANGE, "s")):
        d = sub(res, numKeys=N_BIG, tipo=tipo)
        ax.plot([r["M"] for r in d], [r["bytes"] / 1024 / 1024 for r in d],
                marker=mark, color=color, lw=2, label=tipo)
    ax.set_xscale("log")
    ax.set_xlabel("Ordem M (escala log)")
    ax.set_ylabel("Tamanho do arquivo (MB)")
    ax.set_title("Ocupacao do .dat vs. M  ($N=10^6$)")
    ax.legend(title="Insercao")
    save(fig, "fig_ocupacao.png")


def fig_reuso(reuse):
    """bytes com x sem reuso (barras). Q: ocupacao com e sem reaproveitamento."""
    pares = {}
    for r in reuse:
        key = (r["M"], r["numKeys"])
        pares.setdefault(key, {})[r["reuso"]] = r["bytes"] / 1024 / 1024
    keys = sorted(pares)
    labels = [f"M={m}\nN={n//1000}k" for (m, n) in keys]
    com = [pares[k].get("com", 0) for k in keys]
    sem = [pares[k].get("sem", 0) for k in keys]

    x = range(len(keys))
    w = 0.38
    fig, ax = plt.subplots(figsize=(8.4, 4.2))
    b1 = ax.bar([i - w / 2 for i in x], sem, w, color=ORANGE, label="sem reuso")
    b2 = ax.bar([i + w / 2 for i in x], com, w, color=BLUE, label="com reuso")
    for k, c, s in zip(x, com, sem):
        if s:
            ax.text(k, max(c, s), f"-{(1 - c / s) * 100:.0f}%", ha="center",
                    va="bottom", fontsize=9, color=GREEN, fontweight="bold")
    ax.set_xticks(list(x))
    ax.set_xticklabels(labels)
    ax.set_ylabel("Tamanho final (MB)")
    ax.set_title("Reuso de nos: ocupacao com x sem (churn N -> -N/2 -> +N/2)")
    ax.legend()
    save(fig, "fig_reuso.png")


def fig_io_real(io):
    """tempo_io x M, cacheado vs. restrito (cgroup), Seq e Rand, N=10^6.

    Demonstra que os tempos da bateria padrao sao servidos pelo page cache do
    SO (o .dat cabe na RAM): ao limitar a memoria do processo via cgroup
    (modo 'restrito'), so o padrao de BAIXA LOCALIDADE (Rand) passa a pagar
    I/O real no dispositivo (NVMe); o Sequencial, de alta localidade, quase
    nao muda. As contagens logicas (reads/bytes/altura) sao identicas entre os
    modos -- so o tempo muda. Responde, de forma honesta, a pergunta CPU vs I/O.
    """
    fig, ax = plt.subplots(figsize=(7.6, 4.4))
    series = (
        ("Rand", "restrito", ORANGE, "s", "-", "Rand — restrito (I/O real)"),
        ("Rand", "cacheado", ORANGE, "s", "--", "Rand — cacheado"),
        ("Seq", "restrito", BLUE, "o", "-", "Seq — restrito (I/O real)"),
        ("Seq", "cacheado", BLUE, "o", "--", "Seq — cacheado"),
    )
    for tipo, modo, color, mark, ls, lbl in series:
        d = sorted((r for r in io if r["tipo"] == tipo and r["modo"] == modo),
                   key=lambda r: r["M"])
        if not d:
            continue
        ax.plot([r["M"] for r in d], [r["tempo_io"] for r in d],
                marker=mark, color=color, lw=2, ls=ls, label=lbl)
    ax.set_xscale("log")
    ax.set_xlabel("Ordem M (escala log)")
    ax.set_ylabel("Tempo de I/O (s)")
    ax.set_title("I/O real vs. page cache  ($N=10^6$): cgroup MemoryHigh força o disco")
    ax.legend()
    save(fig, "fig_io_real.png")


def main():
    print("Gerando figuras em slides/img/ ...")
    res = load(os.path.join(DATA, "resultados_experimentos.csv"))
    reuse = load(os.path.join(DATA, "reuso_comparacao.csv"))
    fig_reads_vs_m(res)
    fig_altura(res)
    fig_tradeoff(res)
    fig_ocupacao(res)
    fig_reuso(reuse)
    io_path = os.path.join(DATA, "resultados_io_real.csv")
    if os.path.exists(io_path):
        fig_io_real(load(io_path))
    else:
        print("  (pulado fig_io_real: data/resultados_io_real.csv ausente)")
    print("Pronto.")


if __name__ == "__main__":
    main()

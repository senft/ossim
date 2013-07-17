#!/usr/bin/env python
# encoding: utf-8

import sys
from subprocess import call
import numpy as np
import matplotlib.pyplot as plt


def plot(file):
    call('scavetool vector -p "*hop*" -O hopcount.csv -F csv {0}'.format(file),
         shell=True)

    with open('hopcount.csv') as f:
        v = np.loadtxt(f, delimiter=",", dtype='int', skiprows=1, usecols=(1,))

    bins = sorted(set(n for n in v))
    bins.append(bins[-1] + 1)

    fig, ax = plt.subplots(1, 1)

    ax.xaxis.set_ticks(bins[:-1])

    ax.xaxis.set_label_text('Hopcount')
    ax.yaxis.set_label_text('Probability')

    ax.grid(True)

    ax.hist(v, normed=True, bins=bins, facecolor='green', alpha=0.75,
            align='left', rwidth=6.0/len(bins))

    plt.show()


def main():
    plot(sys.argv[1])

if __name__ == '__main__':
    main()

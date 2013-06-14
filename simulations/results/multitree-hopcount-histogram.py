#!/usr/bin/env python
# encoding: utf-8

from subprocess import call
import numpy as np
import matplotlib.pyplot as plt

def main():
    call('scavetool vector -p "*hop*" -O hopcount.csv -F csv Multitree_Network-0.vec',
            shell=True)

    with open('hopcount.csv') as f:
        v = np.loadtxt(f, delimiter=",", dtype='int', skiprows=1, usecols=(1,))

    # Get rid of hopcounts < 1 (creation of packets is recorded in the source,
    # too)
    v = [n for n in v if n > 0]

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

if __name__ == '__main__':
    main()

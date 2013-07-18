#!/usr/bin/env python2
# encoding: utf-8

import sys
import re
import matplotlib.pyplot as plt
from matplotlib import cm


def read_vector(filename, r_vectors):
    vecnum = 0
    vectors = []
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('vector'):
                v_match = r_vectors.search(line)
                if v_match:
                    vecnum = int(line.split(" ", 3)[1])

            elif line[0].isdigit():
                splitted = line.strip().split("\t")
                cur_vecnum = int(splitted[0])
                if cur_vecnum == vecnum:
                    #print(splitted)
                    value = float(splitted[3])

                    vectors.append(value)

    return vectors


def plot(vectors):
    bins = sorted(set(n for n in vectors))
    bins.append(bins[-1] + 1)

    fig, ax = plt.subplots(1, 1)

    ax.xaxis.set_ticks(bins[:-1])

    ax.xaxis.set_label_text('Hopcount')
    ax.yaxis.set_label_text('Probability')

    ax.grid(True)

    ax.hist(vectors, normed=True, bins=bins, facecolor='green', alpha=0.75,
            align='left', rwidth=6.0/len(bins))

    plt.show()


def main():
    vector = sys.argv[1].decode('string_escape')
    filename = sys.argv[2]

    r_vectors = re.compile(vector)

    vectors = read_vector(filename, r_vectors)
    plot(vectors)

if __name__ == '__main__':
    main()

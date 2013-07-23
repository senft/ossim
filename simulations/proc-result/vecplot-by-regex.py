#!/usr/bin/env python2
# encoding: utf-8

import sys
import re
import matplotlib.pyplot as plt
from matplotlib import cm


def read_vector(filename, r_vectors, r_modules, group_by_vector):

    vectors = dict()

    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('vector'):
                v_match = r_vectors.search(line)
                if v_match:
                    m_match = r_modules.search(line.strip())
                    if m_match:

                        if group_by_vector:
                            label = v_match.group()
                        else:
                            label = m_match.group()

                        vecnum = int(line.split(" ", 3)[1])
                        vectors[vecnum] = {'label': label, 'x': [], 'y': []}

            elif line[0].isdigit():
                splitted = line.strip().split("\t")
                vecnum = int(splitted[0])
                if vecnum in vectors:
                    #print(splitted)
                    time = float(splitted[2])
                    value = float(splitted[3])

                    vectors[vecnum]['x'].append(time)
                    vectors[vecnum]['y'].append(value)

    return vectors


def plot(ylabel, vectors):
    plt.xlabel('Time')
    plt.ylabel(ylabel)

    i = 0
    for vecnum in vectors.keys():
        plt.plot(vectors[vecnum]['x'], vectors[vecnum]['y'], #'--',
                 label=vectors[vecnum]['label'], #marker='o',
                 #color=cm.jet(1.*i/len(vectors))
                 )
        #plt.scatter(vectors[vecnum]['x'], vectors[vecnum]['y'])
        i += 1

    #plt.legend(fontsize=8)
    plt.legend(loc='center left', bbox_to_anchor=(1, 0.5))
    plt.grid()
    plt.show()


def main():
    vector = sys.argv[1].decode('string_escape')
    modules = sys.argv[2].decode('string_escape')
    filename = sys.argv[3]

    group_by_vector = False
    if sys.argv[4] == 'vector':
        group_by_vector = True

    r_vectors = re.compile(vector)
    r_modules = re.compile(modules)

    vectors = read_vector(filename, r_vectors, r_modules, group_by_vector)
    plot(vector, vectors)

if __name__ == '__main__':
    main()

#!/bin/sh

vector_file=$1

./vecplot.py meanOutDegree "$vector_file" &
./vecplot.py bwUtil "$vector_file" &
./vecplot.py forwardingInOne "$vector_file" &
./vecplot.py forwardingInMoreThanOne "$vector_file" &
./vecplot.py meanNumTrees "$vector_file" &
./vecplot.py meanHopcount "$vector_file" &
./vecplot-by-regex.py "nodesActiveIn\d+stripes" globalStatistic $vector_file vector &
./multitree-hopcount-histogram.py $vector_file &
./vecplot-by-regex.py "outDegreeTree\d+" globalStatistic $vector_file vector &
./vecplot-by-regex.py "maxHopcountTree\d+" globalStatistic $vector_file vector &
./vecplot-by-regex.py "meanHopcountTree\d+" globalStatistic $vector_file vector &
./vecplot-by-regex.py "messageCount(CR|DR|CC|PNR|SI)" globalStatistic $vector_file vector &

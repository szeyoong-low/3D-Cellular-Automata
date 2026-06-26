#!/bin/bash
./bin/cellular_automata ./bin/conway_2d.so -d 100x100x1 -s 0.1 -i 1782070284 -t 1 -l
./bin/cellular_automata ./bin/conway.so -d 70x70x70 -s 0.15 -i 1782077142 -t 1 -l
./bin/cellular_automata ./bin/crystal.so -d 50x50x50 -s 0.15 -i 1782070544 -t 1
./bin/cellular_automata ./bin/slime_mold.so -d 50x50x50 -s 0.1 -i 1782072064 -t 1 -o
./bin/cellular_automata ./bin/universe.so -d 70x70x5 -s 0.07 -o
./bin/cellular_automata ./bin/question_rotate.so -d 50x50x50 -s 0.02 -l
./bin/cellular_automata ./bin/cave_2d.so -d 100x100x1 -s 0.5 -l

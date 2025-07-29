#!/bin/bash
perl cinclude2dot.pl --src ../src/ --include ../src/engine/,../src/assets-system/,../src/ecs --exclude .*\.cpp$ | ./python/bin/python3 highlight_edges.py | dot -Tsvg > graph.svg
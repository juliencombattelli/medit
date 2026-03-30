#!/bin/sh

perf record -F 99 -a -g -p $(pidof medit)
perf script > perf.out
stackcollapse-perf.pl perf.out > perf.folded.out
flamegraph.pl perf.folded.out > perf.svg

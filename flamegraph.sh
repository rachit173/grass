#!/bin/bash

RANK=$1
CONFIG_FILE=$2
if [ $# -ne 2 ]; then
    echo "Usage: $0 <rank> <Config filepath>"
    exit 1
fi

FLAME_DIR=/mnt/Work/FlameGraph
BASE_DIR=/mnt/Work/grass
PERF_LOG_DIR=$BASE_DIR/perf_log
RESOURCES_DIR=$BASE_DIR/resources
OUTPUT_DIR=$BASE_DIR/resources/$APP


mkdir -p $PERF_LOG_DIR

echo "Build"
bazel build //src:run_app --copt=-O3

echo "Starting application"
bazel run //src:run_app --copt=-O3 -- $RANK $CONFIG_FILE &
appPid=$!

echo "Recording..."
sudo perf record -F 500 -p $appPid -g -- sleep 40

sudo chmod 666 perf.data

echo "Stack traces"
perf script | $FLAME_DIR/stackcollapse-perf.pl > $PERF_LOG_DIR/out.perf-folded

echo "Plot..."
$FLAME_DIR/flamegraph.pl $PERF_LOG_DIR/out.perf-folded > $PERF_LOG_DIR/perf.svg

echo "Cleanup"
sudo rm perf.data
sudo rm $PERF_LOG_DIR/out.perf-folded
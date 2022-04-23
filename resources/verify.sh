#!/bin/bash


g++ -O3 ./resources/verifier.cpp -o ./resources/verifier


if [ $# -lt 3 ]; then
    echo "Usage: $0 <base_dir> <application> <test_file> <optional arg>"
    exit 1
fi

base_dir=$1
application=$2
test_file=$3
optional=$4

time ./resources/verifier $base_dir $application $test_file $optional
#!/bin/bash


g++ -O3 ./resources/verifier.cpp -o ./resources/verifier


if [ $# -ne 3 ]; then
    echo "Usage: $0 <base_dir> <application> <test_file>"
    exit 1
fi

base_dir=$1
application=$2
test_file=$3

time ./resources/verifier $base_dir $application $test_file
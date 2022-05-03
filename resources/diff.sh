g++ -O3 ./resources/diff.cpp -o ./resources/diff

if [ $# -lt 4 ]; then
    echo "Usage: $0 <base_dir> <application> <test_file> <tolerance>"
    exit 1
fi

base_dir=$1
application=$2
test_file=$3
tolerance=$4

./resources/diff $base_dir $application $test_file $tolerance
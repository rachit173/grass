# Change to base directory
BASE_DIR=/mnt/Work/grass
echo "$BASE_DIR"
cd $BASE_DIR
bazel build -c opt //src:run_app

rm /mnt/Work/grass/tmp_exec.conf
cat >> /mnt/Work/grass/tmp_exec.conf << EOF
buffer.capacity=4
buffer.num_partitions=8
buffer.num_workers=2
buffer.server_addresses=localhost:50051,localhost:50052

app.name=pagerank
app.base_dir=/mnt/Work/grass/resources/graphs
app.iterations=30
app.graph_file=web-BerkStan.txt
EOF
./bazel-bin/src/run_app 0 $BASE_DIR/tmp_exec.conf &
proc1=$(echo $!)
taskset -p $proc1 -c 0,16
echo "Process 1: $proc1"

./bazel-bin/src/run_app 1 $BASE_DIR/tmp_exec.conf &
proc2=$(echo $!)
echo "Process 2: $proc2"
taskset -p $proc2 -c 1,17
wait
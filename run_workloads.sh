# Kill previous instances of run_app
ps -aux | grep run_app | awk '{print $2}' | xargs kill -9

# Change to base directory
BASE_DIR=/mnt/Work/grass
SOCKET_OFFSET=16
echo "$BASE_DIR"
cd $BASE_DIR
bazel build -c opt //src:run_app

RunCore () {
  rank=$1
  ./bazel-bin/src/run_app $rank /mnt/Work/grass/tmp_exec.conf &
  pid=$(echo $!)
  # taskset -p $pid -c $rank,$(($rank+$SOCKET_OFFSET)) 
  echo "Process $rank: $pid"
}

echo "Runnning single machine, 2 cores"
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
RunCore 0
RunCore 1
wait


# echo "Runnning single machine, 4 cores"
# rm /mnt/Work/grass/tmp_exec.conf
# cat >> /mnt/Work/grass/tmp_exec.conf << EOF
# buffer.capacity=2
# buffer.num_partitions=8
# buffer.num_workers=4
# buffer.server_addresses=localhost:40051,localhost:40052,localhost:40053,localhost:40054

# app.name=pagerank
# app.base_dir=/mnt/Work/grass/resources/graphs
# app.iterations=30
# app.graph_file=web-BerkStan.txt
# EOF

# RunCore 0
# RunCore 1
# RunCore 2
# RunCore 3
# wait
# Kill previous instances of run_app
ps -aux | grep run_app | awk '{print $2}' | xargs kill -9

NUM_WORKERS=$1
ITERS=$2
if [ -z $NUM_WORKERS ]; then
  NUM_WORKERS=2
fi
if [ -z $ITERS ]; then
  ITERS=10
fi
# Change to base directory
BASE_DIR=/mnt/Work/grass
SOCKET_OFFSET=16
echo "$BASE_DIR"
cd $BASE_DIR
bazel build --copt=-O3 //src:run_app

KillAll() {
  pids=$(ps -ef | grep run_app | grep -v grep | awk '{print $2}')
  if [ -n "$pids" ]; then
    echo "Killing previous instances of run_app"
    kill -9 $pids
  fi
}

GenerateServerAddresses() {
  local num_workers=$1
  addr=""
  SOCKET_BASE=40000
  for i in $(seq 1 $num_workers); do
    addr+="localhost:$((SOCKET_BASE + i)),"
  done
}

RunCore () {
  rank=$1
  ./bazel-bin/src/run_app $rank /mnt/Work/grass/tmp_exec.conf &
  pid=$(echo $!)
  # taskset -p $pid -c $rank,$(($rank+$SOCKET_OFFSET)) 
  echo "Process $rank: $pid"
}

RunK() {
local num_workers=$1
local iters=$2
GenerateServerAddresses $num_workers
echo "Runnning single machine, $num_workers cores"
rm /mnt/Work/grass/tmp_exec.conf
cat >> /mnt/Work/grass/tmp_exec.conf << EOF
buffer.capacity=4
buffer.num_partitions=$((num_workers*4))
buffer.num_workers=$num_workers
buffer.server_addresses=$addr

app.name=pagerank
app.base_dir=/mnt/Work/grass/resources/graphs
app.out_dir=/mnt/Work/grass/resources
app.iterations=$iters
app.graph_file=web-BerkStan.txt
app.log_level=info
EOF
for (( c=0; c<$num_workers; c++ ))
do
  RunCore $c
done
wait
}

KillAll
RunK $NUM_WORKERS $ITERS
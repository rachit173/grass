# Kill previous instances of run_app
ps -aux | grep run_app | awk '{print $2}' | xargs kill -9

NUM_WORKERS=$1
ITERS=$2
CAPACITY=$3
GRAPH_DATA=$4
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

RunFlameGraph() {
  appPid=$1
  rank=$2

  FLAME_DIR=/mnt/Work/FlameGraph
  PERF_LOG_DIR=$BASE_DIR/perf_log
  mkdir -p $PERF_LOG_DIR

  echo "Recording..."
  sudo perf record -F 500 -p $appPid -g -- sleep 120

  sudo chmod 666 perf.data

  echo "Stack traces"
  perf script | $FLAME_DIR/stackcollapse-perf.pl > $PERF_LOG_DIR/out.perf-folded

  echo "Plot..."
  $FLAME_DIR/flamegraph.pl $PERF_LOG_DIR/out.perf-folded > $PERF_LOG_DIR/perf_$rank.svg

  echo "Cleanup"
  sudo rm perf.data
  sudo rm $PERF_LOG_DIR/out.perf-folded
}

RunCore () {
  rank=$1
  ./bazel-bin/src/run_app $rank /mnt/Work/grass/tmp_exec.conf &
  pid=$(echo $!)

  if [ $rank -eq 0 ]; then
    RunFlameGraph $pid $rank &
  fi
  # taskset -p $pid -c $rank,$(($rank+$SOCKET_OFFSET)) 
  echo "Process $rank: $pid"
}

RunK() {
local num_workers=$1
local iters=$2
local capacity=$3
local graph_data=$4
GenerateServerAddresses $num_workers
echo "Runnning single machine, $num_workers cores"
rm /mnt/Work/grass/tmp_exec.conf
cat >> /mnt/Work/grass/tmp_exec.conf << EOF
buffer.capacity=$capacity
buffer.num_partitions=$((num_workers*4))
buffer.num_workers=$num_workers
buffer.server_addresses=$addr

app.name=pagerank
app.input_dir=/mnt/Work/grass/resources/graphs
app.output_dir=/mnt/Work/grass/resources
app.iterations=$iters
app.graph_file=$graph_data
app.log_level=info
EOF
for (( c=0; c<$num_workers; c++ ))
do
  RunCore $c
done
wait
}

KillAll
RunK $NUM_WORKERS $ITERS $CAPACITY $GRAPH_DATA

# Kill previous instances of run_app

NUM_WORKERS=$1
ITERS=$2
if [ -z $NUM_WORKERS ]; then
  NUM_WORKERS=2
fi
if [ -z $ITERS ]; then
  ITERS=10
fi
NUM_ROWS=$3
if [ -z $NUM_ROWS ]; then
  NUM_ROWS=160
fi
NUM_COLS=$4
if [ -z $NUM_COLS ]; then
  NUM_COLS=240
fi


# Change to base directory
BASE_DIR=/mnt/Work/grass
SOCKET_OFFSET=16
echo "$BASE_DIR"
cd $BASE_DIR
bazel build --copt=-O3 //src/matmul:run_app

KillAll() {
  pids=$(ps -ef | grep run_app | grep -v grep | awk '{print $2}')
  if [ -n "$pids" ]; then
    echo "Killing previous instances of run_app"
    kill -9 $pids
  fi
}

INPUT_FILE=matrix_${NUM_ROWS}_${NUM_COLS}_input.txt

GenerateMatrix() {
  echo "Generating matrix of size ${NUM_ROWS}x${NUM_COLS}"
  python3 $BASE_DIR/tools/generate_matrices.py $NUM_ROWS $NUM_COLS
}

VerifyMatrix() {
  echo "Verifying matrix of size ${NUM_ROWS}x${NUM_COLS}" 
  python3 $BASE_DIR/tools/verify_matrices.py "matrix_${NUM_ROWS}_${NUM_COLS}"
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
  ./bazel-bin/src/matmul/run_app $rank /mnt/Work/grass/tmp_exec.conf &
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
GenerateServerAddresses $num_workers
echo "Runnning single machine, $num_workers cores"
rm /mnt/Work/grass/tmp_exec.conf
cat >> /mnt/Work/grass/tmp_exec.conf << EOF
buffer.capacity=4
buffer.num_partitions=$((num_workers*4))
buffer.num_workers=$num_workers
buffer.server_addresses=$addr

app.name=matmul
app.input_dir=/mnt/Work/grass/resources/matmul
app.output_dir=/mnt/Work/grass/resources
app.iterations=$iters
app.graph_file=$INPUT_FILE
app.log_level=info
EOF
for (( c=0; c<$num_workers; c++ ))
do
  RunCore $c
done
wait
}

KillAll
GenerateMatrix
RunK $NUM_WORKERS $ITERS
VerifyMatrix
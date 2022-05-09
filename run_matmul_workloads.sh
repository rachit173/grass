# Kill previous instances of run_app

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
bazel build --copt=-O3 //src/matmul:run_app

KillAll() {
  pids=$(ps -ef | grep run_app | grep -v grep | awk '{print $2}')
  if [ -n "$pids" ]; then
    echo "Killing previous instances of run_app"
    kill -9 $pids
  fi
}

NUM_ROWS=160
NUM_COLS=240
INPUT_FILE=matrix_${NUM_ROWS}_${NUM_COLS}_input.txt

GenerateMatrix() {
  echo "Generating matrix of size ${NUM_ROWS}x${NUM_COLS}"
  python3 $BASE_DIR/tools/generate_matrices.py $NUM_ROWS $NUM_COLS
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
  ./bazel-bin/src/matmul/run_app $rank /mnt/Work/grass/tmp_exec.conf &
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
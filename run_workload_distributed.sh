NUM_WORKERS=$1
ITERS=$2
CAPACITY=$3
GRAPH_DATA=$4
APP=$5
if [ -z $NUM_WORKERS ]; then
  NUM_WORKERS=2
fi
if [ -z $ITERS ]; then
  ITERS=10
fi
# Change to base directory
BASE_DIR=/mnt/Work/grass
DEPLOY_DIR=$BASE_DIR/deploy
BINARY_DIR=$BASE_DIR/bazel-bin
GRAPH_DATA_DIR=$BASE_DIR/resources/graphs/
TMP_CONFIG_FILE=$BASE_DIR/tmp_exec.conf
SSH_KEY_FILE=/users/ajsj7598/.ssh/id_rsa
SOCKET_OFFSET=16
CURRENT_MACHINE_IP="10.10.1.3"

echo "$BASE_DIR"
pushd $BASE_DIR
bazel build --copt=-O3 //src:run_app

if [ $? -ne 0 ]; then
  echo "Build failed. Exiting..."
  exit 1
fi

StartWorkers() {
  rank=0
  for i in "${addrs_arr[@]}"; do
    echo "Starting worker $rank on $i"
    ssh -i $SSH_KEY_FILE $i "bash $DEPLOY_DIR/start_worker.sh $rank $DEPLOY_DIR" &
    rank=$((rank+1))
  done
}

GenerateServerAddresses() {
  local num_machines=$1
  addrs=""
  addrs_with_ports=""
  IP_BASE="10.10.1."
  SOCKET_BASE=40000

  for i in $(seq 1 $num_machines); do
    MACHINE_IP=$IP_BASE$i
    # for j in $(seq 1 $NUM_WORKERS_PER_MACHINE); do
    addrs+="$MACHINE_IP,"
    addrs_with_ports+="$MACHINE_IP:$((SOCKET_BASE + i)),"
    # done
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

DeployExecutable() {
  local addrs=$1
  
  IFS=',' read -ra addrs_arr <<< "$addrs"
  echo "Deploying to ${addrs_arr[@]}"
  for i in "${addrs_arr[@]}"; do
    echo "Deploying to $i"
    ssh -i $SSH_KEY_FILE $i "rm -rf $DEPLOY_DIR"
    ssh -i $SSH_KEY_FILE $i "mkdir -p $DEPLOY_DIR/bin"
    rsync -e "ssh -i $SSH_KEY_FILE" -r -L $BINARY_DIR/ $i:$DEPLOY_DIR/bin > /dev/null
    rsync -e "ssh -i $SSH_KEY_FILE" $TMP_CONFIG_FILE $i:$DEPLOY_DIR/
    rsync -e "ssh -i $SSH_KEY_FILE" $BASE_DIR/*.sh $i:$DEPLOY_DIR
    # if [ $i != $CURRENT_MACHINE_IP ]; then
    #   rsync -e "ssh -i $SSH_KEY_FILE" -r $GRAPH_DATA_DIR $i:$GRAPH_DATA_DIR
    # fi
  done
}

RunK() {
local num_workers=$1
local iters=$2
local capacity=$3
local graph_data=$4
local app=$5
GenerateServerAddresses $num_workers
echo "Runnning $num_workers worker processes"
rm $TMP_CONFIG_FILE
cat >> $TMP_CONFIG_FILE << EOF
buffer.capacity=$capacity
buffer.num_partitions=$((num_workers*4))
buffer.num_workers=$num_workers
buffer.server_addresses=$addrs_with_ports

app.name=$app
app.input_dir=$BASE_DIR/resources/graphs
app.output_dir=$BASE_DIR/resources
app.iterations=$iters
app.graph_file=$graph_data
app.log_level=info
EOF

DeployExecutable $addrs
StartWorkers
wait
}

# KillAll
RunK $NUM_WORKERS $ITERS $CAPACITY $GRAPH_DATA $APP
bash ./collect_tracing.sh $NUM_WORKERS $GRAPH_DATA $APP
popd

NUM_WORKERS=$1
GRAPH_DATA=$2
APP_NAME=$3
if [ -z $NUM_WORKERS ]; then
  NUM_WORKERS=2
fi

BASE_DIR=/mnt/Work/grass
DEPLOY_DIR=$BASE_DIR/deploy
SSH_KEY_FILE=/users/ajsj7598/.ssh/id_rsa

mkdir -p $BASE_DIR/tracing/

GenerateServerAddresses() {
  local num_machines=$1
  addrs=""
  IP_BASE="10.10.1."
  SOCKET_BASE=40000

  for i in $(seq 1 $num_machines); do
    MACHINE_IP=$IP_BASE$i
    addrs+="$MACHINE_IP,"
  done
}

CopyLogFile() {
  local addrs=$1
  rank=0
  IFS=',' read -ra addrs_arr <<< "$addrs"
  for i in "${addrs_arr[@]}"; do
    echo "Collecting tracing metrics from ${addrs_arr[@]}"
    DEBUG_LOG_FILE="${APP_NAME}_${GRAPH_DATA}${rank}_debug.log"
    rsync -e "ssh -i $SSH_KEY_FILE" $i:$DEPLOY_DIR/logs/$DEBUG_LOG_FILE $BASE_DIR/tracing/$DEBUG_LOG_FILE
    if [ $? -ne 0 ]; then
      echo "Collecting tracing metrics failed. Exiting..."
      exit 1
    fi
    rank=$((rank+1))
  done
}

pushd $BASE_DIR
rm $BASE_DIR/tracing/*.log
GenerateServerAddresses $NUM_WORKERS
CopyLogFile $addrs
python tools/parse_tracing.py $BASE_DIR/tracing ${APP_NAME} ${GRAPH_DATA}

popd
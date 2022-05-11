
RANK=$1
DEPLOY_DIR=$2
pushd $DEPLOY_DIR

RunCore () {
  local rank=$1
  local deploy_dir=$2
  mkdir -p $deploy_dir/logs
  $deploy_dir/bin/src/run_app $rank $deploy_dir/tmp_exec.conf
  pid=$(echo $!)

  echo "Process $rank: $pid"
}

KillAll() {
  pids=$(ps -ef | grep run_app | grep -v grep | awk '{print $2}')
  if [ -n "$pids" ]; then
    echo "Killing previous instances of run_app"
    kill -9 $pids
  fi
}

KillAll
sleep 1
RunCore $RANK $DEPLOY_DIR
popd
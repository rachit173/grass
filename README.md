GRASS: GRaph Analytics Simply Scalable


### Scripts
1. Application - `bazel run //src:run_app -- <rank> <config file>`
2. Flame Graph - `./flamegraph.sh <app> <iters>`
3. Verifier - `./resources/verify.sh <resources dir> <app> <test file>`

#### Examples:
1. Application - `bazel run //src:run_app -- 0 /mnt/Work/grass/resources/exec.conf`
2. Flame Graph - `./flamegraph.sh pagerank 10`
3. Verifier - `./resources/verify.sh /mnt/Work/grass/resources pagerank edges1.txt`



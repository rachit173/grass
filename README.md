# GRASS: GRaph Analytics Simply Scalable
GRASS makes use of a distributed buffer abstraction built on a new data movement algorithm. This distributed buffer abstraction is used to implement algorithms including PageRank, BFS, ConnectedComponents, and Full Attention.

## Runing the 
### Scripts
1. Application - `bazel run //src:run_app -- <rank> <config file>`
2. Flame Graph - `./flamegraph.sh <app> <iters>`
3. Verifier - `./resources/verify.sh <resources dir> <app> <test file>`

#### Examples:
1. Application - `bazel run //src:run_app -- 0 /mnt/Work/grass/resources/exec.conf`
2. Flame Graph - `./flamegraph.sh pagerank 10`
3. Verifier - `./resources/verify.sh /mnt/Work/grass/resources pagerank edges1.txt`

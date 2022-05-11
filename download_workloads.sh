prepend() {
  local filename=$1
  local num_vertices=$2
ed -s $filename << EOF
0a
${num_vertices}
.
w
EOF
}

cd /mnt/Work/grass
wget https://snap.stanford.edu/data/web-BerkStan.txt.gz
gzip -d web-BerkStan.txt.gz
mv web-BerkStan.txt resources/graphs
sed -i '1,4d' /mnt/Work/grass/resources/graphs/web-BerkStan.txt
prepend /mnt/Work/grass/resources/graphs/web-BerkStan.txt 685231

# Graph 500 
rm -rf graphbench
git clone https://github.com/uwsampa/graphbench.git
pushd graphbench/data/generator/
make all
# 2^16 nodes graph with average 8 edges per node
./generator_omp 16 -e 8 -o /mnt/Work/grass/resources/graphs/edges_2_16_e_8.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_16_e_8.txt 65536
# 2^16 nodes graph with average 16 edges per node
./generator_omp 16 -e 16 -o /mnt/Work/grass/resources/graphs/edges_2_16_e_16.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_16_e_16.txt 65536
# 2^16 nodes graph with average 32 edges per node
./generator_omp 16 -e 32 -o /mnt/Work/grass/resources/graphs/edges_2_16_e_32.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_16_e_32.txt 65536
# 2^16 nodes graph with average 64 edges per node
./generator_omp 16 -e 64 -o /mnt/Work/grass/resources/graphs/edges_2_16_e_64.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_16_e_64.txt 65536

# 2^18 nodes graph with average 8 edges per node
./generator_omp 18 -e 8 -o /mnt/Work/grass/resources/graphs/edges_2_18_e_8.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_18_e_8.txt 262144
# 2^18 nodes graph with average 16 edges per node
./generator_omp 18 -e 16 -o /mnt/Work/grass/resources/graphs/edges_2_18_e_16.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_18_e_16.txt 262144
# 2^18 nodes graph with average 32 edges per node
./generator_omp 18 -e 32 -o /mnt/Work/grass/resources/graphs/edges_2_18_e_32.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_18_e_32.txt 262144
# 2^18 nodes graph with average 64 edges per node
./generator_omp 18 -e 64 -o /mnt/Work/grass/resources/graphs/edges_2_18_e_64.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_18_e_64.txt 262144

# 2^20 nodes graph with average 8 edges per node
./generator_omp 20 -e 8 -o /mnt/Work/grass/resources/graphs/edges_2_20_e_8.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_20_e_8.txt 1048576
# 2^20 nodes graph with average 16 edges per node
./generator_omp 20 -e 16 -o /mnt/Work/grass/resources/graphs/edges_2_20_e_16.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_20_e_16.txt 1048576
# 2^20 nodes graph with average 32 edges per node
./generator_omp 20 -e 32 -o /mnt/Work/grass/resources/graphs/edges_2_20_e_32.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_20_e_32.txt 1048576
# 2^20 nodes graph with average 64 edges per node
./generator_omp 20 -e 64 -o /mnt/Work/grass/resources/graphs/edges_2_20_e_64.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_20_e_64.txt 1048576

# 2^24 nodes graph with average 64 edges per node
./generator_omp 24 -e 64 -o /mnt/Work/grass/resources/graphs/edges_2_24_e_64.txt
prepend /mnt/Work/grass/resources/graphs/edges_2_24_e_64.txt 16777216
popd
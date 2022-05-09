cd /mnt/Work/grass
wget https://snap.stanford.edu/data/web-BerkStan.txt.gz
gzip -d web-BerkStan.txt.gz
mv web-BerkStan.txt resources/graphs
sed -i '1,4d' /mnt/Work/grass/resources/graphs/web-BerkStan.txt
sed -i '1s/^/685231\n/' /mnt/Work/grass/resources/graphs/web-BerkStan.txt

# Graph 500 
rm -rf graphbench
git clone https://github.com/uwsampa/graphbench.git
pushd graphbench/data/generator/
make all
# 2^16 nodes graph with average 8 edges per node
./generator_omp 16 -e 8 -o /mnt/Work/grass/resources/graphs/edges_2_16_e_8.txt
sed -i '1s/^/65535\n/' /mnt/Work/grass/resources/graphs/edges_2_16_e_8.txt
# 2^16 nodes graph with average 16 edges per node
./generator_omp 16 -e 16 -o /mnt/Work/grass/resources/graphs/edges_2_16_e_16.txt
sed -i '1s/^/65535\n/' /mnt/Work/grass/resources/graphs/edges_2_16_e_16.txt
# 2^16 nodes graph with average 32 edges per node
./generator_omp 16 -e 32 -o /mnt/Work/grass/resources/graphs/edges_2_16_e_32.txt
sed -i '1s/^/65535\n/' /mnt/Work/grass/resources/graphs/edges_2_16_e_32.txt
# 2^16 nodes graph with average 64 edges per node
./generator_omp 16 -e 64 -o /mnt/Work/grass/resources/graphs/edges_2_16_e_64.txt
sed -i '1s/^/65535\n/' /mnt/Work/grass/resources/graphs/edges_2_16_e_64.txt

# 2^18 nodes graph with average 8 edges per node
./generator_omp 18 -e 8 -o /mnt/Work/grass/resources/graphs/edges_2_18_e_8.txt
sed -i '1s/^/262143\n/' /mnt/Work/grass/resources/graphs/edges_2_18_e_8.txt
# 2^18 nodes graph with average 16 edges per node
./generator_omp 18 -e 16 -o /mnt/Work/grass/resources/graphs/edges_2_18_e_16.txt
sed -i '1s/^/262143\n/' /mnt/Work/grass/resources/graphs/edges_2_18_e_16.txt
# 2^18 nodes graph with average 32 edges per node
./generator_omp 18 -e 32 -o /mnt/Work/grass/resources/graphs/edges_2_18_e_32.txt
sed -i '1s/^/262143\n/' /mnt/Work/grass/resources/graphs/edges_2_18_e_32.txt
# 2^18 nodes graph with average 64 edges per node
./generator_omp 18 -e 64 -o /mnt/Work/grass/resources/graphs/edges_2_18_e_64.txt
sed -i '1s/^/262143\n/' /mnt/Work/grass/resources/graphs/edges_2_18_e_64.txt

# 2^20 nodes graph with average 8 edges per node
./generator_omp 20 -e 8 -o /mnt/Work/grass/resources/graphs/edges_2_20_e_8.txt
sed -i '1s/^/1048575\n/' /mnt/Work/grass/resources/graphs/edges_2_20_e_8.txt
# 2^20 nodes graph with average 16 edges per node
./generator_omp 20 -e 16 -o /mnt/Work/grass/resources/graphs/edges_2_20_e_16.txt
sed -i '1s/^/1048575\n/' /mnt/Work/grass/resources/graphs/edges_2_20_e_16.txt
# 2^20 nodes graph with average 32 edges per node
./generator_omp 20 -e 32 -o /mnt/Work/grass/resources/graphs/edges_2_20_e_32.txt
sed -i '1s/^/1048575\n/' /mnt/Work/grass/resources/graphs/edges_2_20_e_32.txt
# 2^20 nodes graph with average 64 edges per node
./generator_omp 20 -e 64 -o /mnt/Work/grass/resources/graphs/edges_2_20_e_64.txt
sed -i '1s/^/1048575\n/' /mnt/Work/grass/resources/graphs/edges_2_20_e_64.txt

# 2^24 nodes graph with average 64 edges per node
./generator_omp 24 -e 64 -o /mnt/Work/grass/resources/graphs/edges_2_24_e_64.txt
sed -i '1s/^/16777215\n/' /mnt/Work/grass/resources/graphs/edges_2_24_e_64.txt
popd
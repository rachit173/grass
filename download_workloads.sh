

cd /mnt/Work/grass

pushd resources/graphs
# Download the berkStan graph
wget https://snap.stanford.edu/data/web-BerkStan.txt.gz
gzip -d web-BerkStan.txt.gz
popd
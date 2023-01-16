sudo apt install -y  apt-transport-https curl gnupg
curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
sudo mv bazel.gpg /etc/apt/trusted.gpg.d/
echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list

sudo apt update
sudo apt install -y bazel pkg-config libfuse-dev meson python3-pip
pip3 install pytest
echo 'export PATH="$PATH:$HOME/bin"' >> ~/.bashrc


# Installing OR tools
pip3 install networkx
pip3 install matplotlib
pip3 install numpy
pip3 install scipy
sudo apt-get -y install git wget pkg-config build-essential cmake autoconf libtool zlib1g-dev lsb-release
git clone https://github.com/google/or-tools
pushd or-tools
# make cc
# make 
popd

# Install dependencies
mkdir -p third_party
pushd third_party

# OpenBLAS
sudo apt update
sudo apt install -y libopenblas-dev
# sudo update-alternatives --config libblas.so.3

# lapackpp  
git clone https://bitbucket.org/icl/lapackpp.git
pushd lapackpp
make config prefix=$PWD/install
make install
popd
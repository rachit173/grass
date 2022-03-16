sudo apt install -y  apt-transport-https curl gnupg
curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
sudo mv bazel.gpg /etc/apt/trusted.gpg.d/
echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list

sudo apt update
sudo apt install -y bazel pkg-config libfuse-dev meson python3-pip
pip3 install pytest
echo 'export PATH="$PATH:$HOME/bin"' >> ~/.bashrc
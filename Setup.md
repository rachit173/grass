# Instructions for (re)-starting project

1. Generate ssh keys: ssh-keygen -t rsa
2. Print pub key: cat ~/.ssh/id_rsa.pub
3. Add SSH keys to instances (0 —> 1,2,3): On cloudlab
4. Add SSH keys to GitHub
5. Setup with VSCode
6. For all nodes of cluster: do Disk setup, Repo download, repo setup
7. Disk setup 
    1. sudo mkfs.ext4 /dev/sda4
    2. sudo mkdir -p /mnt/Work
    3. sudo mount /dev/sda4 /mnt/Work
    4. sudo chmod 777 /mnt/Work
8. Repo download:
    1. cd /mnt/Work
    2. git clone https://github.com/rachit173/grass.git OR git clone git@github.com:rachit173/grass.git 
    3.  git checkout dev
9. Repo setup:
    1. Download data: bash download_workloads.sh
    2. Run setup script: bash setup.sh
    3. Change SSH key location in run_workload_distributed.sh to appropriate key generated on machine

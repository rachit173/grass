sudo docker run --ipc=host --ulimit memlock=-1 --ulimit stack=67108864  -v /mnt/Work:/workspace/grass --gpus all -it  nvcr.io/nvidia/pytorch:22.05-py3
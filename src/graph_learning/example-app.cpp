#include <torch/torch.h>
#include <iostream>

int main() {
  torch::Device device(torch::kCUDA);
  torch::Tensor tensor = torch::rand({2, 3}).to(device);
  tensor = torch::relu(tensor);
  std::cout << tensor << std::endl;
}
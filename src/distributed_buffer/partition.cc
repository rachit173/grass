#include "distributed_buffer.h"

// #include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <vector>
#include <utility>
#include <fstream>

using graph::VertexPartition;
using graph::InteractionEdges;
using graph::Edge;
using graph::Vertex;
#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif
using namespace std;

int DistributedBuffer::GetStablePartitionId(int round) {
    int outgoing_super_partition = partition_to_be_sent_[round][self_rank_];
    int stable_super_partition = machine_state_[round][self_rank_].first;
    if(machine_state_[round][self_rank_].first == outgoing_super_partition){
        stable_super_partition = machine_state_[round][self_rank_].second;
    }
    return stable_super_partition;
}

// Send partition from hashmap to other server.
graph::VertexPartition* DistributedBuffer::SendPartition(int super_partition_id) {
    graph::VertexPartition* partition;
    while(true) {
        std::unique_lock<std::mutex> lock(mutex_send_);
        for(auto it: done_partitions_) {
            int partition_id = it.first;
            if(partition_id >= super_partition_id * capacity_/2 && partition_id < (super_partition_id + 1) * capacity_/2) {
                partition = it.second;
                cout << "Send partition " << partition_id << "in super partition " << super_partition_id << endl;
                done_partitions_.erase(it.first);
                return partition;
            }
        }
        std::cout << "Waiting on CV to send some partition to another server" << std::endl;
        cv_send_.wait(lock);
    }
  
}

// Write to a file or store to some different buffer to be retrieved when sending to another part of the buffer.
void DistributedBuffer::ReleasePartition(int super_partition_id, int partition_id) {
  graph::VertexPartition* partition = nullptr;
  for(int i = 0; i < capacity_; i++) {
    graph::VertexPartition* vp = vertex_partitions_[i];
    // Can be a nullptr if some partition evicted before.
    if(vp == nullptr) continue;

    if (vp->partition_id() == partition_id) {
      partition = vp;
      vertex_partitions_[i] = nullptr;
      buffer_size_--;
      break;
    }
  }
  std::unique_lock<std::mutex> lock(mutex_send_);
  done_partitions_[partition_id] = partition;
  std::cout << "Release partition: " << partition_id << std::endl;
  cv_send_.notify_one();
}

// Check if the partition is ready to be released.
void DistributedBuffer::CheckAndReleaseOutgoingPartition(int outgoing_super_partition_id, int stable_super_partition_id, int partition_id) {
  // Partition should belong to outgoing super partition to be eligible for release.
  bool partition_eligible_for_release = (partition_id >= outgoing_super_partition_id * (capacity_/2)) && (partition_id < (outgoing_super_partition_id + 1) * (capacity_/2));
  if(!partition_eligible_for_release)
    return;
  
  // std::cout << "Outgoing super partition id: " << outgoing_super_partition_id << ", stable super partition id: " << stable_super_partition_id << ", Partition id: " << partition_id << std::endl;
  bool partition_done = true;
  
  // Check if interactions are complete with the stable super partition as well as inside the outgoing super partition
  int partition_start = stable_super_partition_id * (capacity_ /2), partition_end = (stable_super_partition_id + 1) * (capacity_ /2);
  for(int i = partition_start; i < partition_end; i++) {
    if(! (interactions_matrix_[partition_id][i] && interactions_matrix_[i][partition_id]) ) {
      partition_done = false;
      break;
    }
  }

  // Partition belongs to outgoing super partition. Check interactions with own super partition only for the first round 
  // (As that will only be executed for the first round).
  if(current_round_ == 0) {
    int partition_start2 = outgoing_super_partition_id * (capacity_ /2), partition_end2 = (outgoing_super_partition_id + 1) * (capacity_ /2);
    for(int i = partition_start2; i < partition_end2; i++) {
      if(! (interactions_matrix_[partition_id][i] && interactions_matrix_[i][partition_id]) ) {
        partition_done = false;
        break;
      }
    }
  }

  PrintInteractionMatrix();
  std::cout << "Partition id: " << partition_id << ", partition done: " << partition_done << std::endl;

  // std::cout << "Partition " << partition_id << " done?: " << partition_done << std::endl;
  if(!partition_done) return;

  // Release the outgoing partition.
  ReleasePartition(outgoing_super_partition_id, partition_id);
}

// Add a new partition to the buffer. (vertex_partitions_)
void DistributedBuffer::AddPartitionToBuffer(graph::VertexPartition* partition) {
    for(int i = 0; i < capacity_; i++) {
        if(vertex_partitions_[i] == nullptr) {
            cout << "Adding partition id: " << partition->partition_id() << " to buffer at idx: " << i << endl;
            vertex_partitions_[i] = partition;
            buffer_size_++;
            cout << "Buffer size: " << buffer_size_ << endl;
            return;
        }
    }
}
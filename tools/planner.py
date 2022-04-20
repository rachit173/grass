import argparse
from ortools.init import pywrapinit
from ortools.graph import pywrapgraph
import heapq
import numpy as np
import scipy
import scipy.sparse
import networkx as nx
from pyvis.network import Network

def reduce_to_max_flow(a):
    i, j = a.shape
    n = a.nnz
    data = np.concatenate([2*np.ones(i, dtype=int), np.ones(n + j, dtype=int)])
    indices = np.concatenate([np.arange(1, i + 1),
                              a.indices + i + 1,
                              np.repeat(i + j + 1, j)])
    indptr = np.concatenate([[0],
                             a.indptr + i,
                             np.arange(n + i + 1, n + i + j + 1),
                             [n + i + j]])
    graph = scipy.sparse.csr_matrix((data, indices, indptr), shape=(2+i+j, 2+i+j))
    flow = scipy.sparse.csgraph.maximum_flow(graph, 0, graph.shape[0]-1)
    return flow.flow_value, flow.residual

def matchings(K, edges):
  G = nx.Graph()
  [G.add_edge(e[0], e[1], weight=1) for e in edges]
  G_ = nx.max_weight_matching(G, maxcardinality=True)
  
  pass

def main():
  parser = argparse.ArgumentParser(description='Planner')
  parser.add_argument('--output_path', type=str, default='planned.txt',
                      help='Path to output file')
  parser.add_argument('--num_partitions', type=int, default=10, help='Number of partitions')
  parser.add_argument('--buffer_capacity', type=int, default=2, help='Buffer capacity')
  parser.add_argument('--num_workers', type=int, default=5, help='Number of workers')
  args = parser.parse_args()
  n = args.num_partitions
  k = args.num_workers
  c = args.buffer_capacity
  
  if (n != c*k):
    print("Error: n != c*k")
    return
  K = 2*k


  vertices = [i for i in range(K)]
  G = nx.Graph()
  for i in range(K):
    for j in range(K):
      if i != j:
        G.add_edge(i, j, weight=1)
  iter = 0
  while G.number_of_edges() > 0:
    G_ = nx.max_weight_matching(G, maxcardinality=True)
    # matchings.append(G_)
    edges = []
    for e in G_:
      edges.append((int(e[0]), int(e[1])))
      G.remove_edge(e[0], e[1])
      # G.remove_edge(e[1], e[0])
    print(f'Iter: {iter}')
    print(f'Edges: {edges}')
    print(f'Number of edges: {G.number_of_edges()}')
    print(G.edges())
    iter += 1
  # flow_valuereduce_to_max_flow(G)
  # min_cost_flow = pywrapgraph.SimpleMinCostFlow()
  # src =  K
  # tgt = K+1
  # start_nodes = []
  # end_nodes = []
  # capacities = []
  # unit_costs = []
  # start_nodes.extend([src for i in range(K)])
  # end_nodes.extend([i for i in range(K)])
  # capacities.extend([1 for i in range(K)])
  # unit_costs.extend([1 for i in range(K)])
  # start_nodes.extend([e[0] for e in edges])
  # end_nodes.extend([e[1] for e in edges])
  # capacities.extend([1 for e in edges])
  # unit_costs.extend([1 for e in edges])
  # start_nodes.extend([i for i in range(K)])
  # end_nodes.extend([tgt for i in range(K)])
  # capacities.extend([1 for i in range(K)])
  # unit_costs.extend([1 for i in range(K)])
  # supplies = [0 for i in range(K+2)]
  # supplies[K] = k
  # supplies[K+1] = -k
  # min_cost_flow = pywrapgraph.SimpleMinCostFlow()
  # print(start_nodes)
  # print(end_nodes)
  # print(capacities)
  # print(unit_costs)
  # for arc in zip(start_nodes, end_nodes, capacities, unit_costs):
  #   min_cost_flow.AddArcWithCapacityAndUnitCost(arc[0], arc[1], arc[2], arc[3])
  
  # # Add node supplies.
  # for count, supply in enumerate(supplies):
  #   min_cost_flow.SetNodeSupply(count, supply)
  
  # status = min_cost_flow.Solve()
  # if status != min_cost_flow.OPTIMAL:
  #   print('Optimal solution not found.')
  #   print(f'Status: {status}')
  #   exit(1)
  
  # print('Minimum cost:', min_cost_flow.OptimalCost())
  # print('')
  # print('  Arc    Flow / Capacity  Cost')
  # for i in range(min_cost_flow.NumArcs()):
  #     cost = min_cost_flow.Flow(i) * min_cost_flow.UnitCost(i)
  #     print('%1s -> %1s    %3s   / %3s   %3s' %
  #           (min_cost_flow.Tail(i), min_cost_flow.Head(i),
  #           min_cost_flow.Flow(i), min_cost_flow.Capacity(i), cost))
  with open(args.output_path, 'w') as f:
    f.write(f'{n}\n')
    f.write(f'{c}\n')
    f.write(f'{k}\n')

  pass


if __name__ == '__main__':
  
  main()
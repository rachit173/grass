import networkx as nx

n = 60
G = nx.barabasi_albert_graph(n, 41)
print(n)
for e in G.edges():
  print(e[0], e[1])
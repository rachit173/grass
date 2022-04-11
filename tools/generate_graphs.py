import networkx as nx

BASE_DIR = "/mnt/Work/grass/resources/graphs/"

def generate_graph(n):
  G = nx.gnp_random_graph(n, 0.5, directed=True)

  with open(BASE_DIR + "/edges.txt", "w") as f:
    f.write(str(n) + "\n")
    for e in G.edges():
      f.write(str(e[0]) + " " + str(e[1]) + "\n")
  
  return G

def generate_pagerank(graph):
  nstart = {}
  for n in graph.nodes():
    nstart[n] = 1.0

  pr = nx.pagerank(graph, alpha=0.85, nstart=nstart)
  with open(BASE_DIR + "/pagerank.txt", "w") as f:
    for node in pr:
      f.write(str(node) + " " + str(pr[node]) + "\n")

n = 3
graph = generate_graph(n)
generate_pagerank(graph)
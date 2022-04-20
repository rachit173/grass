#include <bits/stdc++.h>
#include <cmath>
using namespace std;
typedef vector<vector<pair<int, int>>> vvii;
void generate_matchings(int l, int r, vvii& matchings) {
  cout << "l: " << l << " r: " << r << endl;
  if ((r-l) == 2) {
    vector<pair<int, int>> tmp;
    tmp.push_back(make_pair(l, l+1));
    matchings.push_back(tmp);
    return;
  }
  int mid = l + (r-l)/2;
  vvii left_half;
  // left half
  generate_matchings(l, mid, left_half);
  vvii right_half;
  // right half
  generate_matchings(mid, r, right_half);
  // merge
  assert(left_half.size() == right_half.size());
  if (left_half.size() > 0) {
    for (int i = 0; i < left_half.size(); i++) {
      auto tmp = left_half[i];
      for (auto& x: right_half[i]) {
        tmp.push_back(x);
      }
      matchings.push_back(tmp); 
    }
  }

  int n = (r-l);  
  // generate matchings using the bipartite graph
  for (int z = 0; z < n/2; z++) {
    vector<pair<int, int>> m;
    for (int i = 0; i < n/2; i++) {
      m.push_back(make_pair(l+i, mid+(i+z)%(n/2)));
    }
    matchings.push_back(m);
  }
}


bool verify_matchings(int m, vvii& matchings) {
  int n2 = matchings.size();
  if (n2 != m-1) return false;
  int n1 = (m * (m-1))/2;
  set<pair<int, int>> edges;
  for (auto& matching: matchings) {
    for (auto& edge: matching) {
      edges.insert(edge);
    }
  }
  if (edges.size() == n1) {
    return true;
  }
  return false;
}
int main(int argc, char** argv) {
  int k = atoi(argv[1]);
  int m = 2*k;
  if ((1 << int(log2(m))) != m) {
    cout << "m must be a power of 2, m: " << m <<  endl;    
  }
  cout << m << endl;
  vvii matchings;
  generate_matchings(0, m, matchings);
  cout << "Number of matchings: " << matchings.size() << endl;
  for (auto& matching: matchings) {
    for (auto& x: matching) {
      cout << "(" << x.first << ", " << x.second << "), ";
    }
    cout << endl;
  }
  if (verify_matchings(m, matchings)) {
    cout << "Matchings are correct" << endl;
  } else {
    cout << "Matchings have errors" << endl;
  }
  return 0;
}
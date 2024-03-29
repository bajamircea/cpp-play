- general
  - use symbol table to map to vertex index
  - V vertices
  - E edges
    - max V^2
- undirected
  - representation
    - adjacency list (many args don't care of order)
      - space E + V, if sparse that's much smaller than V^2
    - adjacency matrix: space V^2, only good for dense graphs
    - set of edges
  - traversal:
    - DFS: recurse or use stack
    - BFS: use queue
    - both use arrays `int marked[]` and `int edgeTo[]` of size V
      - note `edgeTo[w] = v` means the relevant edge is from `v` to `w`
      - also can have `int distanceTo[]`
        - in this case the distance is the edge count,
          later will have weights
  - connected components
    - uses DFS and `int id[]` of size V
- directed = digraph
  - representation
    - real-world digraphs tend to be sparse
  - DFS, BFS same as for undirected
  - topological sort:
    - A > B, C > B, A > C => [A, C, B]
    - loop vertices:
      - if not marked:
        - use DFS
        - store on stack in visit post order
    - reverse stack
  - strongly-connected components (i.e. paths on both directions)
    - Kosaraju-Sharir algorithm:
      - idea:
        - topological order of the reversed graph
        - run DFS on the reverse topological order
      - easy to implement, complex correctness explanation
- undirected edge weighted
  - MST theory:
    - MST = minimum spanning tree
      - spanning (includes all vertices)
      - tree (connected, acyclic)
      - uses V-1 edges
      - minimum, uses edge weight
    - given any cut, the crossing edge of min weight is in MST
  - Krushal's MST: E logE
    - sort edged by weight (use min heap)
    - add edge unless cycle (union find to determine if cycle `log*V`)
  - Prim's MST: E logE
    - start at vetex 0 and follow outgoing min weight vertex (use min heap)
    - lazy
    - eager
    - indexed min heap
      - can decrease key
      - knows index size beforehand (number of vertices)
  - Boruvka's MST (not shown)
- directed edge weighted
  - shortest path theory:
    - from to kinds:
      - single source `s` to all
        - algs below for this case
      - source to sink
      - all pairs of vertices
    - restrictions on weights:
      - non-negative
      - Euclidiean
      - arbitrary
    - cycles
      - yes
      - no directed cycles
      - no negative weight cycles
        - if negative weights are allowed
        - no shortest path exists
    - single source to all representation:
      - arrays `int edgeTo[]` and `double distTo[]` of size V
      - "relax edge" means adjust them for the "to" vertex when a shorter path is found
  - Dijkstra's
    - suitable for: non-negative weights
    - idea
      - consider vertices in increasing distance from `s`
      - add vertex to tree and relax all edges from that vertex
      - use indexed min heap for vertices based on distance from `s`
    - complexity: V insert, V delete-min, E decrease key
      - typical use binary heap: E log E
      - worth using d-way heap for performance
      - not worth Fibonacci heap
      - 2D array: optimal for dense graphs: V^2
  - Topological sort based
    - suitable for: no directed cycles (i.e. edge-weighted DAGs)
    - idea
      - consider vertices in topological order
      - relax all edges from that index
    - complexity: E + V
    - for longest path negate weights and result at the end
      - applications: critical path in Gannt graphs
  - Bellman-Ford
    - suitable for: no negative weight cycles (fewer restrictions of the three)
    - idea
      - double loop for each vertex
      - relax all edges from that index
    - dynamic programming (not greedy)
    - complexity: ExV
    - there is a queue based Bellman-Ford that takes typical case to E + V
      - maintain a queue of vertices where the distance changed
        - and only traverse those at the next step
    - can be used to find a negative cycle
- max flow and min cut
  - edges have capacity (simpler pb. if integral) e.g. 9/10
    - undirected graph (e.g. reference to the same edge)
  - keep on searching for an augmeting path
    - e.g. using a "residual" graph 9/10 edge becomes:
      - edge of 1
      - back edge of 9
#pragma once
#include <cstring>
#include <cmath>
#include "CsrGraphMulti.h"

using std::vector;


struct CompressedTrees {
  int num_rows;
  int fvs_size;
  int original_nodes;
  int chunk_size;
  int **tree_rows; //rowoffsets
  int **tree_cols; //columns
  int **edge_offset; //offset of the corresponding edge in the csr format
  int **parent;      //parent array corresponding to each edge
  int **distance;    //distance array in terms of unweighted edge.
  int **precompute_value; //This is used to store the precomputed value corresponding to each tree.
  int **nodes_index; //This is used to store the index of the nodes corresponding to ith position.
  int **test_temp;
  CsrGraphMulti *parent_graph;
  int *final_vertices;  //contains the final fvs vertices.
  int *vertices_map; //contains the index of the fvs vertices and -1 if the vertex doesn't belong to fvs.
  int *(*pinned_memory_allocator)(int, int);
  void (*free_pinned_memory)(int *);
  bool pinned_memory;

  CompressedTrees(int chunk, int N, int *fvs_array, CsrGraphMulti *graph) {
    fvs_size = N;
    chunk_size = chunk;
    parent_graph = graph;
    original_nodes = graph->Nodes;
    pinned_memory = false;
    int r = CEILDIV(N, chunk_size);
    num_rows = r;
    tree_rows = new int*[num_rows];
    tree_cols = new int*[num_rows];
    edge_offset = new int*[num_rows];
    parent = new int*[num_rows];
    distance = new int*[num_rows];
    nodes_index = new int*[num_rows];
    precompute_value = new int*[num_rows];
    test_temp = new int*[num_rows];

    for (int i = 0; i < num_rows; i++) {
      tree_rows[i] = new int[chunk * (original_nodes + 1)];
      tree_cols[i] = new int[chunk * original_nodes];
      edge_offset[i] = new int[chunk * original_nodes];
      parent[i] = new int[chunk * original_nodes];
      distance[i] = new int[chunk * original_nodes];
      nodes_index[i] = new int[chunk * original_nodes];
      precompute_value[i] = new int[chunk * original_nodes];
      memset(tree_rows[i], 0, sizeof(int) * chunk * (original_nodes + 1));
    }

    final_vertices = new int[fvs_size];
    vertices_map = fvs_array;
    for (int i = 0; i < original_nodes; i++)
      if (vertices_map[i] != -1) {
        assert(vertices_map[i] < fvs_size);
        final_vertices[vertices_map[i]] = i;
      }
  }

  CompressedTrees(int chunk, int N, int *fvs_array, CsrGraphMulti *graph,
      int *(*mem_alloc)(int, int), void (*mem_free)(int *)) {
    fvs_size = N;
    chunk_size = chunk;
    parent_graph = graph;
    original_nodes = graph->Nodes;
    pinned_memory = true;
    pinned_memory_allocator = mem_alloc;
    free_pinned_memory = mem_free;
    int r = CEILDIV(N, chunk_size);
    num_rows = r;
    tree_rows = new int*[num_rows];
    tree_cols = new int*[num_rows];
    edge_offset = new int*[num_rows];
    parent = new int*[num_rows];
    distance = new int*[num_rows];
    nodes_index = new int*[num_rows];
    precompute_value = new int*[num_rows];
    test_temp = new int*[num_rows];

    for (int i = 0; i < num_rows; i++) {
      tree_rows[i] = pinned_memory_allocator(chunk, original_nodes + 1);
      tree_cols[i] = pinned_memory_allocator(chunk, original_nodes);
      edge_offset[i] = (int*) pinned_memory_allocator(chunk, original_nodes);
      parent[i] = new int[chunk * original_nodes];
      distance[i] = new int[chunk * original_nodes];
      nodes_index[i] = new int[chunk * original_nodes];
      precompute_value[i] = pinned_memory_allocator(chunk, original_nodes);
      memset(tree_rows[i], 0, sizeof(int) * chunk * (original_nodes + 1));
    }

    final_vertices = new int[fvs_size];
    vertices_map = fvs_array;
    for (int i = 0; i < original_nodes; i++)
      if (vertices_map[i] != -1) {
        assert(vertices_map[i] < fvs_size);
        final_vertices[vertices_map[i]] = i;
      }
  }

  void clear_memory() {
    for (int i = 0; i < num_rows; i++) {
      if (pinned_memory) {
        free_pinned_memory(tree_rows[i]);
        free_pinned_memory(tree_cols[i]);
        free_pinned_memory((int *) edge_offset[i]);
        free_pinned_memory(precompute_value[i]);
      } else {
        delete[] tree_rows[i];
        delete[] tree_cols[i];
        delete[] edge_offset[i];
        delete[] precompute_value[i];
      }
      delete[] parent[i];
      delete[] distance[i];
      delete[] nodes_index[i];
    }
    delete[] tree_rows;
    delete[] tree_cols;
    delete[] parent;
    delete[] edge_offset;
    delete[] precompute_value;
    delete[] distance;
    delete[] final_vertices;
    delete[] nodes_index;
  }

  int get_node_arrays(int **csr_rows, int **csr_cols,
      int **csr_edge_offset, int **csr_parent, int **csr_distance,
      int node_index);

  int get_node_arrays_warp(int **csr_rows, int **csr_cols,
      int **csr_edge_offset, int **csr_parent, int **csr_distance,
      int **csr_nodes_index, int node_index);

  int get_precompute_array(int **precompute_tree, int node_index);

  int get_index(int original_node);

  void copy(int index, vector<int> *tree_edges,
      vector<int> *parent_edges, vector<int> *distances);

  void print_tree();
};

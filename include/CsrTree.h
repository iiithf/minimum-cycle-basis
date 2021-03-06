#pragma once
#include <assert.h>
#include "CsrGraphMulti.h"
#include "Dijkstra.h"

using std::vector;


class CsrTree {
public:
  int root;
  CsrGraphMulti *parent_graph;
  vector<int> *tree_edges;
  vector<int> *non_tree_edges = NULL;
  vector<int> *parent_edges;
  vector<int> *distance;

  struct compare {
    CsrGraphMulti *parent_graph;
    compare(CsrGraphMulti *graph) {
      parent_graph = graph;
    }
    bool operator()(const int &A, const int &B) const {
      return (parent_graph->weights->at(A) < parent_graph->weights->at(B));
    }
  };

  CsrTree(CsrGraphMulti *graph) {
    parent_graph = graph;
    assert(parent_graph != NULL);
    assert(parent_graph->rows->size() == parent_graph->cols->size());
    assert(parent_graph->rowOffsets->size() == parent_graph->Nodes + 1);
  }

  ~CsrTree() {
    tree_edges->clear();
    non_tree_edges->clear();
    parent_edges->clear();
    distance->clear();
    tree_edges = NULL;
    non_tree_edges = NULL;
    parent_edges = NULL;
    distance = NULL;

  }

  void populate_tree_edges(bool populate_non_tree_edges, int &src) {
    if (populate_non_tree_edges)
      non_tree_edges = new vector<int>();
    root = src;
    tree_edges = parent_graph->get_spanning_tree(&non_tree_edges, src);
    //sort(non_tree_edges->begin(),non_tree_edges->end(),compare(parent_graph));
  }

  void obtain_shortest_path_tree(Dijkstra &helper, bool populate_non_tree_edges, int src) {
    printf("obtain_shortest_path_tree: %d (+1)\n", src+1);
    if (populate_non_tree_edges)
      non_tree_edges = new vector<int>();

    root = src;
    helper.dijkstra_sp(src);
    helper.compute_non_tree_edges(&non_tree_edges);
    tree_edges = helper.tree_edges;
    parent_edges = new vector<int>();

    for (int i = 0; i < helper.edge_offsets.size(); i++)
      parent_edges->push_back(helper.edge_offsets[i]);

    distance = new vector<int>();
    for (int i = 0; i < helper.distance.size(); i++)
      distance->push_back(helper.distance[i]);
  }

  inline void get_edge_endpoints(int &row, int &col, int &weight, int &offset) {
    parent_graph->getEdge(offset, row, col, weight);
  }

  vector<int> *get_parent_edges() {
    vector<int> *v = new vector<int>();
    assert(tree_edges != NULL);
    int row, col;
    v->at(root) = -1;

    for (int i = 0; i < tree_edges->size(); i++) {
      int edge = tree_edges->at(i);
      row = parent_graph->rows->at(i);
      col = parent_graph->cols->at(i);
      v->at(col) = row;
    }
    return v;
  }

  void print_distance() {
    printf("=================================================================================\n");
    printf("Printing Distance,count = %d\n", distance->size());
    for (int i = 0; i < distance->size(); i++)
      printf("-> %d : %d\n", i+1, distance->at(i));
    printf("=================================================================================\n");
  }

  void print_tree_edges() {
    printf("=================================================================================\n");
    printf("Printing Spanning Tree Edges,count = %d\n", tree_edges->size());
    for (int i = 0; i < tree_edges->size(); i++)
      printf("%u %u %d\n", parent_graph->rows->at(tree_edges->at(i)) + 1,
          parent_graph->cols->at(tree_edges->at(i)) + 1,
          parent_graph->weights->at(tree_edges->at(i)));
    printf("=================================================================================\n");
  }

  void print_non_tree_edges() {
    if (non_tree_edges == NULL) return;
    printf("=================================================================================\n");
    printf("Printing Non-Tree Edges,count = %d\n", non_tree_edges->size());
    for (int i = 0; i < non_tree_edges->size(); i++)
      printf("%u %u\n", parent_graph->rows->at(non_tree_edges->at(i)) + 1,
          parent_graph->cols->at(non_tree_edges->at(i)) + 1);
    printf("=================================================================================\n");
  }
};

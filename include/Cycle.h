#pragma once
#include <utility>
#include <assert.h>
#include "BitVector.h"
#include "CompressedTrees.h"

using std::vector;


struct Cycle {
  CompressedTrees *trees;
  int root;
  int non_tree_edge_index;
  int total_length;
  int ID;

  bool operator<(const Cycle &rhs) const {
    return (total_length < rhs.total_length);
  }

  struct compare {
    bool operator()(Cycle *lhs, Cycle *rhs) {
      return (lhs->total_length < rhs->total_length);
    }
  };

  Cycle(CompressedTrees *tr, int root, int index) {
    trees = tr;
    non_tree_edge_index = index;
    this->root = root;
  }

  int get_root() {
    return root;
  }

  // set<int> *get_edges()
  // {
  //   set<int> *edges = new set<int>();
  //   csr_multi_graph *parent_graph = tree->parent_graph;
  //   int row = parent_graph->rows->at(non_tree_edge_index);
  //   int col = parent_graph->columns->at(non_tree_edge_index);
  //   while(row != tree->root)
  //   {
  //     int edge_offset = tree->parent_edges->at(row);
  //     int reverse_edge_offset = parent_graph->reverse_edge->at(edge_offset);
  //     edges->insert(min(edge_offset,reverse_edge_offset));
  //     row = parent_graph->rows->at(edge_offset);
  //   }
  //   while(col != tree->root)
  //   {
  //     int edge_offset = tree->parent_edges->at(col);
  //     int reverse_edge_offset = parent_graph->reverse_edge->at(edge_offset);
  //     edges->insert(min(edge_offset,reverse_edge_offset));
  //     col = parent_graph->rows->at(edge_offset);
  //   }
  //   edges->insert(min(non_tree_edge_index,parent_graph->reverse_edge->at(non_tree_edge_index)));
  //   return edges;
  // }

  /**
   * @brief
   * This method returns a bit_vector corresponding to the edges of the cycle.
   * @details
   * The cycles are represented using bit_vectors of non_tree edges. Non_tree
   * edges present in the cycles are marked set in the bit_vectors at their
   * corresponding positions.
   * @param non_tree_edges map of non_tree edges and its position from 0 - non_tree_edges.size() - 1
   * @return bit_vector describing the cycle.
   */
  BitVector *get_cycle_vector(vector<int> &non_tree_edges, int num_elements) {
    BitVector *vector = new BitVector(num_elements);
    int row = trees->parent_graph->rows->at(non_tree_edge_index);
    int col = trees->parent_graph->cols->at(non_tree_edge_index);

    //set flag for the current edge
    if (non_tree_edges[non_tree_edge_index] >= 0)
      vector->set(non_tree_edges[non_tree_edge_index], true);

    int *node_rowoffsets, *node_columns, edge_offset, *nodes_index;
    int *node_edgeoffsets, *node_parents, *node_distance;
    int src_index = trees->get_index(root);

    trees->get_node_arrays_warp(&node_rowoffsets, &node_columns,
        &node_edgeoffsets, &node_parents, &node_distance, &nodes_index, src_index);

    //check for vertices row =====> root.
    while (node_parents[row] != -1) {
      edge_offset = node_parents[row];
      if (non_tree_edges[edge_offset] >= 0)
        vector->set(non_tree_edges[edge_offset], true);
      if (trees->parent_graph->rows->at(edge_offset) != row)
        row = trees->parent_graph->rows->at(edge_offset);
      else
        row = trees->parent_graph->cols->at(edge_offset);
      assert(row != -1);
    }

    //check for vertices col =====> root.
    while (node_parents[col] != -1) {
      edge_offset = node_parents[col];
      if (non_tree_edges[edge_offset] >= 0)
        vector->set(non_tree_edges[edge_offset], true);
      if (trees->parent_graph->rows->at(edge_offset) != col)
        col = trees->parent_graph->rows->at(edge_offset);
      else
        col = trees->parent_graph->cols->at(edge_offset);
      assert(col != -1);
    }
    return vector;
  }

  /**
   * @brief
   * This method returns a bit_vector corresponding to the edges of the cycle.
   * @details The cycles are represented using bit_vectors of non_tree edges.
   * Non_tree edges present in the cycles are marked set in the bit_vectors at
   * their corresponding positions.
   * @param non_tree_edges map of non_tree edges and its position from 0 - non_tree_edges.size() - 1
   * @return bit_vector describing the cycle.
   */
  void *get_cycle_vector(vector<int> &non_tree_edges, int num_elements, BitVector *cycle_vector) {
    cycle_vector->init();
    int row = trees->parent_graph->rows->at(non_tree_edge_index);
    int col = trees->parent_graph->cols->at(non_tree_edge_index);

    //set flag for the current edge
    if (non_tree_edges[non_tree_edge_index] >= 0)
      cycle_vector->set(non_tree_edges[non_tree_edge_index], true);

    int *node_rowoffsets, *node_columns, edge_offset, *nodes_index;
    int *node_edgeoffsets, *node_parents, *node_distance;
    int src_index = trees->get_index(root);
    trees->get_node_arrays_warp(&node_rowoffsets, &node_columns,
        &node_edgeoffsets, &node_parents, &node_distance, &nodes_index, src_index);

    //check for vertices row =====> root.
    while (node_parents[row] != -1) {
      edge_offset = node_parents[row];
      if (non_tree_edges[edge_offset] >= 0)
        cycle_vector->set(non_tree_edges[edge_offset], true);
      if (trees->parent_graph->rows->at(edge_offset) != row)
        row = trees->parent_graph->rows->at(edge_offset);
      else
        row = trees->parent_graph->cols->at(edge_offset);
      assert(row != -1);
    }

    //check for vertices col =====> root.
    while (node_parents[col] != -1) {
      edge_offset = node_parents[col];
      if (non_tree_edges[edge_offset] >= 0)
        cycle_vector->set(non_tree_edges[edge_offset], true);
      if (trees->parent_graph->rows->at(edge_offset) != col)
        col = trees->parent_graph->rows->at(edge_offset);
      else
        col = trees->parent_graph->cols->at(edge_offset);
      assert(col != -1);
    }
  }

  void print() {
    printf("=================================================================================\n");
    printf("Root is %u\n", root + 1);
    printf("Edge is %u - %u\n",
        trees->parent_graph->rows->at(non_tree_edge_index) + 1,
        trees->parent_graph->cols->at(non_tree_edge_index) + 1);
    printf("Total weight = %d\n", total_length);
    printf("=================================================================================\n");
  }

  void print_line() {
    printf("{%u,(%u - %u)} ", root + 1,
        trees->parent_graph->rows->at(non_tree_edge_index) + 1,
        trees->parent_graph->cols->at(non_tree_edge_index) + 1);
  }
};

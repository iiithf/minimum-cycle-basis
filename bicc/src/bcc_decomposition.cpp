#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <utility>
#include <set>
#include <stack>
#include <omp.h>
#include <string>
#include <algorithm>
#include <atomic>
#include <list>
#include <unordered_set>

#include "bicc.h"
#include "dfs.h"
#include "connected_component.h"
#include "dfs_helper.h"
#include "FileReader.h"
#include "Files.h"
#include "utils.h"
#include "HostTimer.h"
#include "CsrGraph.h"

using std::string;
using std::list;
using std::vector;
using std::unordered_set;
using std::unordered_map;


Debugger dbg;
HostTimer timer;
string InputFileName;
string OutputFileDirectory;
int outputFiles = 0;
int edgesRemoved = 0;
int bridges = 0;
double totalTime = 0;
bool keepBridges = 1;


int main(int argc, char* argv[]) {
  if (argc < 5) {
    printf("Ist Argument should indicate the InputFile\n");
    printf("2nd Argument should indicate the outputdirectory\n");
    printf("3rd Argument should indicate the degree of pruning into bccs.\n");
    printf("4th Argument should indicate the number of nodes.\n");
    printf("5th Argument should indicate whether we should keep the bridges. i.e. 0 or 1 (True default)\n");
    printf("6th argument should indicate the number of threads.(Optional) (1 default)\n");
    exit(1);
  }

  int num_threads = 1;
  keepBridges = atoi(argv[5]);
  if (argc == 7)
    num_threads = atoi(argv[6]);
  omp_set_num_threads(num_threads);

  InputFileName = argv[1];
  OutputFileDirectory = argv[2];
  int degree_pruning = atoi(argv[3]);
  int global_nodes_count = atoi(argv[4]);

  //Read the Inputfile.
  string InputFilePath = InputFileName;
  FileReader Reader(InputFilePath.c_str());

  int v1, v2, Initial_Vertices, weight;
  int nodes, edges;

  //firt line of the input file contains the number of nodes and edges
  Reader.get_nodes_edges(nodes, edges);
  bicc_graph *graph = new bicc_graph(nodes);

  /*
   * ====================================================================================
   * Fill Edges.
   * ====================================================================================
   */
  for (int i = 0; i < edges; i++) {
    Reader.read_edge(v1, v2, weight);
    graph->insert_edge(v1, v2, weight, false);
  }
  graph->calculate_nodes_edges();
  graph->initialize_bicc_numbers();
  Reader.close();
  debug("Input File Reading Complete...\n");

  /*
   * ====================================================================================
   * Each File specific Initialization.
   * ====================================================================================
   */
  int component_number = 1;
  int new_component_number = 1;
  //This datastructure is used to hold edge_lists corresponding to each component number
  unordered_map<int, list<int>*> edge_list_component;
  //This datastructure is used to hold the vertex lists corresponding to each component number.
  unordered_map<int, int> src_vtx_component;
  //Initialize the starting source vertex for component 1.
  src_vtx_component[1] = 0;

  /*
   * ====================================================================================
   * Edge_Map stores the mapping from <src,dest> => edge_index for the initial graph.
   * ====================================================================================
   */
  unordered_map<uint64_t, int> *edge_map = create_map(graph->c_graph);

  /*
   * ====================================================================================
   * Vector for dfs_helper. Number of elements in dfs_helper = min(number of components,4).
   * Initially, only one dfs_helper is required.
   * ====================================================================================
   */
  vector<dfs_helper*> vec_dfs_helper;
  for (int i = 0; i < num_threads; i++)
    vec_dfs_helper.push_back(new dfs_helper(global_nodes_count));
  debug("Initialization of the graph completed.\n");

  /*
   * =========================================================================================
   * Invoke connected_component for the first run on the Input Graph.
   * ==========================================================================================
   */
  int num_components = obtain_connected_components(component_number,
      new_component_number, graph, vec_dfs_helper[0], edge_map);

  debug("Obtained the Initial Connected Components :", num_components);
  edge_list_component.clear();
  src_vtx_component.clear();

  /*
   * =========================================================================================
   * For every component within the range [start,end], collect the edge_lists and collect one 
   * source vertex.
   * ==========================================================================================
   */
  graph->collect_edges_component(component_number + 1, new_component_number,
      edge_list_component, src_vtx_component);

  double _counter_init = timer.start();
  assert(!src_vtx_component.empty());
  bool flag = true;
  /*
   * =========================================================================================
   * Follow the above steps in a loop, until we cannot remove any edge from any component.
   * The finished components are the components whose nodes have degree higher than
   * the filter threshold.
   * ==========================================================================================
   */
  unordered_set<int> finished_components;
  vector<int> component_list;

  for (auto&& it : edge_list_component) {
    component_list.push_back(it.first);
  }

  int num_iterations = 0;
  double time_dfs = 0;
  double time_pruning = 0;

  while (flag) {
    num_iterations++;
    flag = false;
    component_number = new_component_number;
    edge_list_component.clear();
    double _local_time_dfs = timer.start();

#pragma omp parallel for
    for (int i = 0; i < component_list.size(); i++) {
      int thread_id = omp_get_thread_num();

      if (finished_components.find(component_list[i])
          != finished_components.end())
        continue;

      //debug("Active component DFS:",component_list[i],src_vtx_component[component_list[i]] + 1);

      int num_bridges = dfs_bicc_initializer(
          src_vtx_component[component_list[i]], component_list[i],
          new_component_number, graph, vec_dfs_helper[thread_id],
          edge_map, edge_list_component, keepBridges);

      bridges += num_bridges;
    }

    time_dfs += (timer.stop() - _local_time_dfs);
    src_vtx_component.clear();

    /*
     * ====================================================================
     * Structures required for parallel computation. 
     * Each omp thread works on an individual component for the pruning part.
     * For each thread. we add the component numbers which cannot be further pruned to the 
     * list_finished_components.
     * 
     * component_list just contains the list of component numbers collected from the 
     * above run.
     * ====================================================================
     */
    list<int> list_finished_components[num_threads]; //This list is used to hold the finished component numbers for num_threads
    component_list.clear();

    for (auto&& it : edge_list_component) {
      int edge_end_point = graph->c_graph->rows->at(it.second->front());
      src_vtx_component[it.first] = edge_end_point;
      component_list.push_back(it.first);
    }

    //debug("Size of Component_list:",component_list.size());

    /*
     * ====================================================================
     * Parallely prune the edge lists and update the finished component 
     * ====================================================================
     */
    double _local_time_pruning = timer.start();

#pragma omp parallel for
    for (int i = 0; i < component_list.size(); i++) {
      int thread_id = omp_get_thread_num();
      //debug("Active component Prune:",component_list[i],src_vtx_component[component_list[i]] + 1);

      if (finished_components.find(component_list[i]) != finished_components.end())
        continue;

      int num_edges = 0;
      num_edges += graph->prune_edges(degree_pruning, component_list[i],
          edge_list_component[component_list[i]], edge_map, src_vtx_component);

      if (num_edges == 0)
        list_finished_components[thread_id].push_back(component_list[i]);

      edgesRemoved += num_edges;
      //debug("number of edges removed:",thread_id,num_edges);

      if (num_edges != 0) {
#pragma omp critical
        flag = 1;
      }
    }

    time_pruning += (timer.stop() - _local_time_pruning);

    /*
     * ====================================================================
     * Insert the finished component Ids into the finished components list.
     * ====================================================================
     */
    for (int i = 0; i < num_threads; i++) {
      for (auto&& it : list_finished_components[i]) {
        finished_components.insert(it);
      }
      list_finished_components[i].clear();
    }

  }

  debug("Num Iterations:", num_iterations);
  double _counter_exit = timer.stop();
  totalTime += (_counter_exit - _counter_init);

  debug("Total Number of Components in the current file =",
      finished_components.size());
  graph->print_to_a_file(outputFiles, OutputFileDirectory,
      global_nodes_count, finished_components);

  delete graph;
  edge_list_component.clear();
  src_vtx_component.clear();
  vec_dfs_helper.clear();

  debug("Total dfs time:", time_dfs);
  debug("Total pruning time:", time_pruning);
  debug("Total Edges Removed:", edgesRemoved);
  debug("Total Number of Bridges", bridges);
  debug("Total Number of components", outputFiles);
  printf("%d\n", edgesRemoved);
  printf("%lf\n", totalTime);
  return 0;
}


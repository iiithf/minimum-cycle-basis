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
#include <unordered_map>

#include "FileReader.h"
#include "Files.h"
#include "utils.h"
#include "HostTimer.h"
#include "CsrGraph.h"
#include "CsrTree.h"

using std::string;
using std::vector;
using std::unordered_map;


Debugger dbg;
HostTimer timer;
string InputFileName;
string OutputFileName;
double totalTime = 0;
unordered_map<int, int> forward_order;

int main(int argc, char* argv[]) {
  if (argc < 4) {
    printf("Ist Argument should indicate the InputFile\n");
    printf("2nd Argument should indicate the OutputFileName\n");
    printf("3th argument should indicate the number of threads.(Optional) (1 default)\n");
    exit(1);
  }

  int num_threads = 1;
  if (argc == 4)
    num_threads = atoi(argv[3]);
  InputFileName = argv[1];
  OutputFileName = argv[2];
  omp_set_num_threads(num_threads);

  //Open the FileReader class
  string InputFilePath = InputFileName;
  //Read the Inputfile.
  FileReader Reader(InputFilePath.c_str());

  int v1, v2, Initial_Vertices, weight;
  int nodes, edges;
  //firt line of the input file contains the number of nodes and edges
  Reader.get_nodes_edges(nodes, edges);
  int count = 0;

  vector<vector<int> > edge_lists;

  for (int i = 0; i < edges; i++) {
    Reader.read_edge(v1, v2, weight);
    if (forward_order.find(v1) == forward_order.end())
      forward_order[v1] = count++;

    if (forward_order.find(v2) == forward_order.end())
      forward_order[v2] = count++;

    edge_lists.push_back(vector<int>());
    edge_lists[i].push_back(v1);
    edge_lists[i].push_back(v2);
    edge_lists[i].push_back(weight);

  }
  vector<int> ArticulationPoints;
  FILE *input_file = Reader.file;
  int count_ap = 0, curr_ap;
  fscanf(input_file, "%d", &count_ap);

  //push to Articulation Points
  for (int i = 0; i < count_ap; i++) {
    fscanf(input_file, "%d", &curr_ap);
    ArticulationPoints.push_back(curr_ap - 1);
  }
  Reader.close();

  FileWriter fout(OutputFileName.c_str(), forward_order.size(), edges);
  for (int i = 0; i < edges; i++) {
    fout.write_edge(forward_order[edge_lists[i][0]],
        forward_order[edge_lists[i][1]], edge_lists[i][2]);
    edge_lists[i].clear();
  }

  FILE *file = fout.file;
  fprintf(file, "%d\n", count_ap);
  for (int i = 0; i < count_ap; i++)
    fprintf(file, "%d\n", forward_order[ArticulationPoints[i]] + 1);
  fprintf(file, "%d\n", -1);
  fprintf(file, "%d\n", nodes);
  fprintf(file, "%d\n", forward_order.size());
  ArticulationPoints.clear();

  for (auto&& it : forward_order) {
    fprintf(file, "%u %u\n", it.first+1, it.second+1);
  }

  fout.close();
  edge_lists.clear();
  forward_order.clear();
  return 0;
}

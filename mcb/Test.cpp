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

#include "FileReader.h"
#include "Files.h"
#include "utils.h"
#include "Host_Timer.h"
#include "CsrGraph.h"
#include "CsrTree.h"
#include "bit_vector.h"
#include "work_per_thread.h"

debugger dbg;
HostTimer globalTimer;

std::string InputFileName;
std::string OutputFileDirectory;

double totalTime = 0;

int main(int argc,char* argv[])
{
	if(argc < 4)
	{
		printf("Ist Argument should indicate the InputFile\n");
		printf("2nd Argument should indicate the outputdirectory\n");
		printf("3th argument should indicate the number of threads.(Optional) (1 default)\n");
		exit(1);
	}

	int num_threads = 1;

	if(argc == 4)
		num_threads = atoi(argv[3]);

	InputFileName = argv[1];

    	omp_set_num_threads(num_threads);

    	//Open the FileReader class
	std::string InputFilePath = InputFileName;

	//Read the Inputfile.
	FileReader Reader(InputFilePath.c_str());

	int v1,v2,Initial_Vertices,weight;;

	int nodes,edges;

	//firt line of the input file contains the number of nodes and edges
	Reader.get_nodes_edges(nodes,edges); 

	csr_graph *graph=new csr_graph();

	graph->Nodes = nodes;
	/*
	 * ====================================================================================
	 * Fill Edges.
	 * ====================================================================================
	 */
	for(int i=0;i<edges;i++)
	{
		Reader.read_edge(v1,v2,weight);
		graph->insert(v1,v2,weight,false);
	}

	graph->calculateDegreeandRowOffset();

	Reader.fileClose();

	std::vector<std::vector<unsigned> > *chains = new std::vector<std::vector<unsigned> >();

	debug("Input File Reading Complete...\n");

	int source_vertex;

	std::vector<unsigned> *remove_edge_list = graph->mark_degree_two_chains(&chains,source_vertex);
	//initial_spanning_tree.populate_tree_edges(true,NULL,souce_vertex);

	std::vector<std::vector<unsigned> > *edges_new_list = new std::vector<std::vector<unsigned> >();

	int nodes_removed = 0;

	for(int i=0;i<chains->size();i++)
	{
		unsigned row,col;
		unsigned total_weight = graph->sum_edge_weights(chains->at(i),row,col);

		nodes_removed += chains->at(i).size() - 1;

		std::vector<unsigned> new_edge = std::vector<unsigned>();
		new_edge.push_back(row);
		new_edge.push_back(col);
		new_edge.push_back(total_weight);

		edges_new_list->push_back(new_edge);
		//debug(row+1,col+1,total_weight);
	}

	debug ("Number of nodes removed = ",nodes_removed);

	csr_multi_graph *reduced_graph = csr_multi_graph::get_modified_graph(graph,
									     remove_edge_list,
									     edges_new_list,
									     nodes_removed);

	//Node Validity
	assert(reduced_graph->Nodes + nodes_removed == graph->Nodes);

	reduced_graph->print_graph();

	csr_tree *initial_spanning_tree = new csr_tree(reduced_graph);
	initial_spanning_tree->populate_tree_edges(true,source_vertex);

	int num_non_tree_edges = initial_spanning_tree->non_tree_edges->size();

	//Spanning Tree Validity
	assert(num_non_tree_edges == reduced_graph->rows->size()/2 - reduced_graph->Nodes + 1);

	initial_spanning_tree->print_tree_edges();
	initial_spanning_tree->print_non_tree_edges();


	worker_thread **multi_work = new worker_thread*[num_threads];
	for(int i=0;i<num_threads;i++)
		multi_work[i] = new worker_thread(reduced_graph);

	//produce shortest path trees across all the nodes.
	#pragma omp parallel for 
	for(int i = 0; i < reduced_graph->Nodes; i++)
	{
		int threadId = omp_get_thread_num();
		multi_work[threadId]->produce_sp_tree_and_cycles(i,reduced_graph);
	}

	std::vector<cycle*> list_cycle;

	//block
	{
		int space[num_threads] = {0};
		for(int i=0;i<num_threads;i++)
			space[i] = multi_work[i]->list_cycles.size();

		int prev = 0;
		for(int i=0;i<num_threads;i++)
		{
			int temp = space[i];
			space[i] = prev;
			prev += temp;
		}

		//total number of cycles;
		list_cycle.resize(prev);

		#pragma omp parallel for
		for(int i=0;i<num_threads;i++)
		{
			int threadId = omp_get_thread_num();
			for(int j=0;j<multi_work[i]->list_cycles.size();j++)
				list_cycle[space[i] + j] = multi_work[i]->list_cycles[j];

			multi_work[i]->empty_cycles();

		}

	}

	std::sort(list_cycle.begin(),list_cycle.end(),cycle::compare());

	printf("List Cycles\n");
	for(int i=0;i<list_cycle.size();i++)
	{
		printf("%u-(%u - %u) : %d\n",list_cycle[i]->get_root() + 1,
					reduced_graph->rows->at(list_cycle[i]->non_tree_edge_index) + 1,
					reduced_graph->columns->at(list_cycle[i]->non_tree_edge_index) + 1,
					list_cycle[i]->total_length);
	}

	//At this stage we have the shortest path trees and the cycles sorted in increasing order of length.


	//generate the bit vectors
	bit_vector **support_vectors = new bit_vector*[num_non_tree_edges];
	for(int i=0;i<num_non_tree_edges;i++)
	{
		*support_vectors = new bit_vector(num_non_tree_edges);
		(*support_vectors)->set_bit(i,true);
	}



	return 0;
}
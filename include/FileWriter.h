#pragma once
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "utils.h"
#include "mmio.h"


struct FileWriter {
  FILE *file;


  FileWriter(const char *name, int verts, int edges) {
    MM_typecode code;
    file = fopen(name, "w");
    ASSERTMSG(file, "Unable to open file: %s\n", name);
    mm_initialize_typecode(&code);
    mm_set_matrix(&code);
    mm_set_coordinate(&code);
    mm_set_integer(&code);
    mm_set_symmetric(&code);
    mm_write_banner(file, code);
    mm_write_mtx_crd_size(file, verts, verts, edges);
  }


  void write_edge(int u, int v, int wt) {
    fprintf(file, "%d %d %d\n", u+1, v+1, wt);
  }


  void close() {
    fclose(file);
  }
};

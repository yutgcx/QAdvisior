#pragma once

#include "dependency.hpp"

using namespace std;

// A parallel group (gates that execute in same cycle)
struct ParallelGroup {
    int* gate_ids;          // Array of gate IDs in this group
    int num_gates;
    int cycle;              // Cycle number (0-indexed)
};

// Scheduling result
struct Schedule {
    ParallelGroup** groups;  // Array of parallel groups
    int num_groups;          // Number of cycles
    int* gate_to_cycle;      // Mapping: gate_id -> cycle number
    float speedup;           // Theoretical speedup (original_depth / num_groups)
};

// Function declarations
Schedule* create_schedule(int num_gates);
void free_schedule(Schedule* schedule);
Schedule* compute_parallel_schedule(DependencyGraph* graph);
void print_schedule(Schedule* schedule, DependencyGraph* graph);

#include "scheduler.hpp"
#include <iostream>

using namespace std;

Schedule* create_schedule(int num_gates) {
    Schedule* schedule = new Schedule();
    if (!schedule) {
        cerr << "ERROR: Failed to allocate schedule" << endl;
        return nullptr;
    }
    
    schedule->groups = nullptr;
    schedule->num_groups = 0;
    schedule->gate_to_cycle = new int[num_gates];
    for (int i = 0; i < num_gates; i++) {
        schedule->gate_to_cycle[i] = -1;
    }
    schedule->speedup = 1.0;
    
    return schedule;
}

void free_schedule(Schedule* schedule) {
    if (!schedule) return;
    
    if (schedule->groups) {
        for (int i = 0; i < schedule->num_groups; i++) {
            if (schedule->groups[i]) {
                delete[] schedule->groups[i]->gate_ids;
                delete schedule->groups[i];
            }
        }
        delete[] schedule->groups;
    }
    delete[] schedule->gate_to_cycle;
    delete schedule;
}

// Greedy scheduling algorithm
Schedule* compute_parallel_schedule(DependencyGraph* graph) {
    if (!graph || graph->num_gates == 0) return nullptr;
    
    Schedule* schedule = create_schedule(graph->num_gates);
    
    // Track which gates are scheduled
    int* scheduled = new int[graph->num_gates]();
    
    // Track ready gates (all dependencies satisfied)
    int* ready = new int[graph->num_gates];
    int ready_count = 0;
    
    // Initialize: gates with no dependencies are ready
    for (int i = 0; i < graph->num_gates; i++) {
        if (graph->nodes[i].num_dependencies == 0) {
            ready[ready_count++] = i;
        }
    }
    
    int cycle = 0;
    
    while (ready_count > 0) {
        // Create group for this cycle
        ParallelGroup* group = new ParallelGroup();
        group->gate_ids = new int[ready_count];
        group->num_gates = ready_count;
        group->cycle = cycle;
        
        // Copy ready gates to group
        for (int i = 0; i < ready_count; i++) {
            int gate_id = ready[i];
            group->gate_ids[i] = gate_id;
            scheduled[gate_id] = 1;
            schedule->gate_to_cycle[gate_id] = cycle;
        }
        
        // Add group to schedule
        schedule->num_groups++;
        ParallelGroup** new_groups = new ParallelGroup*[schedule->num_groups];
        for (int i = 0; i < schedule->num_groups - 1; i++) {
            new_groups[i] = schedule->groups[i];
        }
        new_groups[schedule->num_groups - 1] = group;
        delete[] schedule->groups;
        schedule->groups = new_groups;
        
        // Find newly ready gates for next cycle
        int* new_ready = new int[graph->num_gates];
        int new_ready_count = 0;
        
        for (int i = 0; i < graph->num_gates; i++) {
            if (!scheduled[i]) {
                // Check if all dependencies are scheduled
                int all_deps_scheduled = 1;
                for (int j = 0; j < graph->nodes[i].num_dependencies; j++) {
                    int dep_id = graph->nodes[i].depends_on[j];
                    if (!scheduled[dep_id]) {
                        all_deps_scheduled = 0;
                        break;
                    }
                }
                if (all_deps_scheduled) {
                    new_ready[new_ready_count++] = i;
                }
            }
        }
        
        // Update ready array
        delete[] ready;
        ready = new_ready;
        ready_count = new_ready_count;
        cycle++;
    }
    
    // Calculate speedup
    int original_depth = graph->num_gates;
    int optimized_depth = schedule->num_groups;
    schedule->speedup = (float)original_depth / optimized_depth;
    
    delete[] scheduled;
    delete[] ready;
    
    return schedule;
}


void print_schedule(Schedule* schedule, DependencyGraph* graph) {
    if (!schedule || !graph) {
        cout << "Schedule or graph is NULL" << endl;
        return;
    }
    
    cout << "\n=== Parallel Execution Schedule ===" << endl;
    cout << "Sequential depth: " << graph->num_gates << " cycles" << endl;
    cout << "Optimized depth:  " << schedule->num_groups << " cycles" << endl;
    cout << "Speedup:          " << schedule->speedup << "x" << endl << endl;
    
    for (int g = 0; g < schedule->num_groups; g++) {
        ParallelGroup* group = schedule->groups[g];
        cout << "Cycle " << group->cycle << ": [";
        
        for (int i = 0; i < group->num_gates; i++) {
            int gate_id = group->gate_ids[i];
            cout << *(graph->gates[gate_id]->gate_string);
            if (i < group->num_gates - 1) {
                cout << ", ";
            }
        }
        cout << "]" << endl;
    }
    
    // Mark parallel groups in the original sequence
    cout << "\n=== Gate-to-Cycle Mapping ===" << endl;
    for (int i = 0; i < graph->num_gates; i++) {
        cout << "Gate " << i << ": cycle " << schedule->gate_to_cycle[i] 
             << " (" << *(graph->gates[i]->gate_string) << ")" << endl;
    }
    cout << "\n================================" << endl;
}

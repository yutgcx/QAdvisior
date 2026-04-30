#pragma once

#include "../lexer/ast.hpp"
#include <string>

using namespace std;

// Gate information extracted from AST for analysis
struct GateInfo {
    int gate_id;                    // Unique ID (0..n-1)
    GateType type;                  // H, CX, MEASURE, etc.
    int* qubits_used;               // Array of qubit indices this gate touches
    int num_qubits;                 // 1 for H/X/Z, 2 for CX, 1 for MEASURE
    int classical_bit;              // For MEASURE: -1 if none, else bit index
    int line_number;                // Source line for error reporting
    string* gate_string;            // Human readable (for debugging)
};

// Dependency graph node
struct DependencyNode {
    int gate_id;
    int* depends_on;                // Array of gate IDs this gate depends on
    int num_dependencies;
    int* needed_by;                 // Array of gate IDs that depend on this gate
    int num_needed_by;
    int execution_cycle;            // Assigned cycle (-1 = not assigned)
    int in_degree;                  // For topological sort
};

// Complete dependency graph
struct DependencyGraph {
    GateInfo** gates;               // Array of gate info (indexed by gate_id)
    int num_gates;
    DependencyNode* nodes;          // Array of nodes (indexed by gate_id)
    int** adjacency_matrix;         // Optional: for visualization
};

// Function declarations
DependencyGraph* create_dependency_graph(int num_gates);
void free_dependency_graph(DependencyGraph* graph);

// Extract gate info from AST
GateInfo** extract_gate_info(ProgramAST* ast, int* num_gates);
string* gate_type_to_string(GateType type);
void print_gate_info(GateInfo* info);

// Build dependencies
void build_dependencies(DependencyGraph* graph);
int share_qubits(GateInfo* a, GateInfo* b);
int share_classical_bits(GateInfo* a, GateInfo* b);
void add_dependency(DependencyGraph* graph, int gate_id, int depends_on_id);

// Query functions
int* get_gates_on_qubit(DependencyGraph* graph, int qubit, int* count);
int has_dependency(DependencyGraph* graph, int gate_id, int potential_dep);

// Debug functions
void print_dependency_graph(DependencyGraph* graph);
void export_graphviz(DependencyGraph* graph, const char* filename);


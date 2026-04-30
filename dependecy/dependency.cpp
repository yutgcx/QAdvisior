#include "dependency.hpp"
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>

using namespace std;

// Custom error reporting functions
static void report_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    cerr << "ERROR: ";
    vfprintf(stderr, format, args);
    cerr << endl;
    va_end(args);
}

static void report_warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    cerr << "WARNING: ";
    vfprintf(stderr, format, args);
    cerr << endl;
    va_end(args);
}

// Create new dependency graph
DependencyGraph* create_dependency_graph(int num_gates) {
    DependencyGraph* graph = new DependencyGraph();
    if (!graph) {
        report_error("Failed to allocate dependency graph");
        return nullptr;
    }
    
    graph->num_gates = num_gates;
    graph->gates = new GateInfo*[num_gates]();
    graph->nodes = new DependencyNode[num_gates]();
    
    // Initialize nodes
    for (int i = 0; i < num_gates; i++) {
        graph->nodes[i].gate_id = i;
        graph->nodes[i].depends_on = nullptr;
        graph->nodes[i].num_dependencies = 0;
        graph->nodes[i].needed_by = nullptr;
        graph->nodes[i].num_needed_by = 0;
        graph->nodes[i].execution_cycle = -1;
        graph->nodes[i].in_degree = 0;
    }
    
    // Allocate adjacency matrix (optional, for visualization)
    graph->adjacency_matrix = new int*[num_gates];
    for (int i = 0; i < num_gates; i++) {
        graph->adjacency_matrix[i] = new int[num_gates]();
    }
    
    return graph;
}

// Free dependency graph
void free_dependency_graph(DependencyGraph* graph) {
    if (!graph) return;
    
    for (int i = 0; i < graph->num_gates; i++) {
        if (graph->gates[i]) {
            delete[] graph->gates[i]->qubits_used;
            delete graph->gates[i]->gate_string;
            delete graph->gates[i];
        }
        if (graph->nodes[i].depends_on) {
            delete[] graph->nodes[i].depends_on;
        }
        if (graph->nodes[i].needed_by) {
            delete[] graph->nodes[i].needed_by;
        }
        if (graph->adjacency_matrix[i]) {
            delete[] graph->adjacency_matrix[i];
        }
    }
    delete[] graph->adjacency_matrix;
    delete[] graph->gates;
    delete[] graph->nodes;
    delete graph;
}

// Helper function to create formatted string
static string* create_formatted_string(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return new string(buffer);
}

// Extract gate information from Phase 1 AST
GateInfo** extract_gate_info(ProgramAST* ast, int* num_gates) {
    if (!ast) {
        report_error("AST is NULL");
        return nullptr;
    }
    
    // First, count the gates
    int count = 0;
    GateNode* current = ast->gates;
    while (current) {
        count++;
        current = current->next;
    }
    *num_gates = count;
    
    if (count == 0) {
        report_warning("No gates found in circuit");
        return nullptr;
    }
    
    // Allocate array
    GateInfo** gates = new GateInfo*[count];
    
    // Fill gate info
    current = ast->gates;
    int idx = 0;
    while (current) {
        GateInfo* info = new GateInfo();
        info->gate_id = idx;
        info->type = current->type;
        info->line_number = 0; // Would come from lexer
        info->classical_bit = -1;
        
        // Allocate qubits array
        switch(current->type) {
            case GateType::HADAMARD:
            case GateType::PAULI_X:
            case GateType::PAULI_Z:
                info->num_qubits = 1;
                info->qubits_used = new int[1];
                if (current->target) {
                    info->qubits_used[0] = current->target->index;
                } else {
                    info->qubits_used[0] = -1;
                }
                info->gate_string = create_formatted_string("%s %s[%d]",
                    gate_type_to_string(current->type)->c_str(),
                    current->target ? current->target->reg_name->c_str() : "?",
                    current->target ? current->target->index : -1);
                break;
                
            case GateType::CX:
                info->num_qubits = 2;
                info->qubits_used = new int[2];
                if (current->control) {
                    info->qubits_used[0] = current->control->index;  // Control
                }
                if (current->target) {
                    info->qubits_used[1] = current->target->index;   // Target
                }
                info->gate_string = create_formatted_string("CX %s[%d], %s[%d]",
                    current->control ? current->control->reg_name->c_str() : "?",
                    current->control ? current->control->index : -1,
                    current->target ? current->target->reg_name->c_str() : "?",
                    current->target ? current->target->index : -1);
                break;
                
            case GateType::MEASURE:
                info->num_qubits = 1;
                info->qubits_used = new int[1];
                if (current->target) {
                    info->qubits_used[0] = current->target->index;
                }
                // Parse classical bit from string like "c[0]"
                if (current->classical_bit) {
                    info->classical_bit = stoi(current->classical_bit->substr(2));
                }
                info->gate_string = create_formatted_string("MEASURE %s[%d] -> %s",
                    current->target ? current->target->reg_name->c_str() : "?",
                    current->target ? current->target->index : -1,
                    current->classical_bit ? current->classical_bit->c_str() : "?");
                break;
                
            case GateType::GATE_BARRIER:
                info->num_qubits = 0;
                info->qubits_used = nullptr;
                info->gate_string = new string("BARRIER");
                break;
                
            default:
                info->num_qubits = 0;
                info->qubits_used = nullptr;
                info->gate_string = new string("UNKNOWN");
                break;
        }
        
        gates[idx++] = info;
        current = current->next;
    }
    
    return gates;
}

// Check if two gates share any qubit
int share_qubits(GateInfo* a, GateInfo* b) {
    if (!a || !b) return 0;
    
    for (int i = 0; i < a->num_qubits; i++) {
        for (int j = 0; j < b->num_qubits; j++) {
            if (a->qubits_used[i] == b->qubits_used[j] && 
                a->qubits_used[i] != -1) {
                return 1;  // Shared qubit found
            }
        }
    }
    return 0;
}

// Check if two measurements share classical bits
int share_classical_bits(GateInfo* a, GateInfo* b) {
    if (a->type == GateType::MEASURE && b->type == GateType::MEASURE) {
        return (a->classical_bit == b->classical_bit && 
                a->classical_bit != -1);
    }
    return 0;
}

// Add dependency: gate_id depends on depends_on_id
void add_dependency(DependencyGraph* graph, int gate_id, int depends_on_id) {
    DependencyNode* node = &graph->nodes[gate_id];
    
    // Check if dependency already exists
    for (int i = 0; i < node->num_dependencies; i++) {
        if (node->depends_on[i] == depends_on_id) {
            return;  // Already exists
        }
    }
    
    // Add to depends_on list
    int* new_depends_on = new int[node->num_dependencies + 1];
    for (int i = 0; i < node->num_dependencies; i++) {
        new_depends_on[i] = node->depends_on[i];
    }
    new_depends_on[node->num_dependencies] = depends_on_id;
    delete[] node->depends_on;
    node->depends_on = new_depends_on;
    node->num_dependencies++;
    node->in_degree++;
    
    // Add to needed_by list of the previous gate
    DependencyNode* prev_node = &graph->nodes[depends_on_id];
    int* new_needed_by = new int[prev_node->num_needed_by + 1];
    for (int i = 0; i < prev_node->num_needed_by; i++) {
        new_needed_by[i] = prev_node->needed_by[i];
    }
    new_needed_by[prev_node->num_needed_by] = gate_id;
    delete[] prev_node->needed_by;
    prev_node->needed_by = new_needed_by;
    prev_node->num_needed_by++;
    
    // Update adjacency matrix (for visualization)
    graph->adjacency_matrix[depends_on_id][gate_id] = 1;
}

// Build all dependencies for the graph
void build_dependencies(DependencyGraph* graph) {
    if (!graph || graph->num_gates == 0) return;
    
    // For each gate, check all previous gates for conflicts
    for (int i = 0; i < graph->num_gates; i++) {
        for (int j = 0; j < i; j++) {
            // Gate i depends on gate j if they share qubits OR share classical bits
            if (share_qubits(graph->gates[i], graph->gates[j]) ||
                share_classical_bits(graph->gates[i], graph->gates[j])) {
                add_dependency(graph, i, j);
            }
        }
    }
}

// Get all gates that touch a specific qubit
int* get_gates_on_qubit(DependencyGraph* graph, int qubit, int* count) {
    if (!graph || qubit < 0) return nullptr;
    
    int* result = new int[graph->num_gates];
    *count = 0;
    
    for (int i = 0; i < graph->num_gates; i++) {
        GateInfo* info = graph->gates[i];
        for (int j = 0; j < info->num_qubits; j++) {
            if (info->qubits_used[j] == qubit) {
                result[(*count)++] = i;
                break;
            }
        }
    }
    
    return result;
}

// Check if gate_id depends on potential_dep
int has_dependency(DependencyGraph* graph, int gate_id, int potential_dep) {
    if (!graph || gate_id >= graph->num_gates || potential_dep >= graph->num_gates) {
        return 0;
    }
    
    DependencyNode* node = &graph->nodes[gate_id];
    for (int i = 0; i < node->num_dependencies; i++) {
        if (node->depends_on[i] == potential_dep) {
            return 1;
        }
    }
    return 0;
}

// Convert GateType to string
string* gate_type_to_string(GateType type) {
    switch(type) {
        case GateType::HADAMARD: return new string("H");
        case GateType::PAULI_X: return new string("X");
        case GateType::PAULI_Z: return new string("Z");
        case GateType::CX: return new string("CX");
        case GateType::MEASURE: return new string("MEASURE");
        case GateType::GATE_BARRIER: return new string("BARRIER");
        default: return new string("UNKNOWN");
    }
}

// Print gate info (debugging)
void print_gate_info(GateInfo* info) {
    if (!info) return;
    cout << "Gate " << info->gate_id << ": " << *(info->gate_string) << endl;
    cout << "  Qubits: [";
    for (int i = 0; i < info->num_qubits; i++) {
        cout << info->qubits_used[i] << (i < info->num_qubits - 1 ? ", " : "");
    }
    cout << "]" << endl;
    if (info->classical_bit != -1) {
        cout << "  Classical bit: " << info->classical_bit << endl;
    }
}

// Print full dependency graph
void print_dependency_graph(DependencyGraph* graph) {
    if (!graph) {
        cout << "Dependency graph is NULL" << endl;
        return;
    }
    
    cout << "\n=== Dependency Graph ===" << endl;
    cout << "Total gates: " << graph->num_gates << endl << endl;
    
    for (int i = 0; i < graph->num_gates; i++) {
        print_gate_info(graph->gates[i]);
        
        DependencyNode* node = &graph->nodes[i];
        if (node->num_dependencies > 0) {
            cout << "  Depends on: ";
            for (int j = 0; j < node->num_dependencies; j++) {
                cout << node->depends_on[j] << " ";
            }
            cout << endl;
        }
        
        if (node->num_needed_by > 0) {
            cout << "  Needed by: ";
            for (int j = 0; j < node->num_needed_by; j++) {
                cout << node->needed_by[j] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
    cout << "========================" << endl;
}

// Export to GraphViz DOT format
void export_graphviz(DependencyGraph* graph, const char* filename) {
    if (!graph) return;
    
    FILE* f = fopen(filename, "w");
    if (!f) {
        report_error("Cannot open file for GraphViz export: %s", filename);
        return;
    }
    
    fprintf(f, "digraph Dependencies {\n");
    fprintf(f, "  rankdir=TB;\n");
    fprintf(f, "  node [shape=box, style=filled, fillcolor=lightblue];\n\n");
    
    // Create nodes
    for (int i = 0; i < graph->num_gates; i++) {
        fprintf(f, "  gate%d [label=\"%d: %s\"];\n", 
                i, i, graph->gates[i]->gate_string->c_str());
    }
    
    fprintf(f, "\n");
    
    // Create edges (dependencies)
    for (int i = 0; i < graph->num_gates; i++) {
        DependencyNode* node = &graph->nodes[i];
        for (int j = 0; j < node->num_dependencies; j++) {
            fprintf(f, "  gate%d -> gate%d;\n", node->depends_on[j], i);
        }
    }
    
    fprintf(f, "}\n");
    fclose(f);
    
    cout << "GraphViz export written to: " << filename << endl;
    cout << "To render: dot -Tpng " << filename << " -o graph.png" << endl;
}
#pragma once

#include<bits/stdc++.h>

using namespace std;

enum class GateType {
    HADAMARD,
    PAULI_X,
    PAULI_Z,
    CX,
    MEASURE,
    GATE_BARRIER
};

enum class NodeType {
    PROGRAM,
    GATE,
    QREG_DECL,
    CREG_DECL,
    MEASURE
};


struct Qubit {
    string* reg_name;     
    int index;          
};


struct GateNode {
    GateType type;
    Qubit* target;     
    Qubit* control;     // For CX only, nullptr otherwise
    string* classical_bit; // For MEASURE: e.g., "c[0]"
    GateNode* next;     // Linked list for multiple gates
};


struct QregDecl {
    string* name;
    int size;
    QregDecl* next;
};


struct CregDecl {
    string* name;
    int size;
    CregDecl* next;
};

// Program AST
struct ProgramAST {
    QregDecl* qubit_registers;
    CregDecl* classical_registers;
    GateNode* gates;
};

// Function declarations
GateNode* create_gate_node(GateType type, Qubit* target, Qubit* control);
GateNode* create_measure_node(Qubit* target, string* classical_bit);
Qubit* create_qubit(string* reg_name, int index);
QregDecl* create_qreg_decl(string* name, int size);
CregDecl* create_creg_decl(string* name, int size);
ProgramAST* create_program_ast(void);

void add_gate_to_program(ProgramAST* ast, GateNode* gate);
void add_qreg_to_program(ProgramAST* ast, QregDecl* qreg);
void add_creg_to_program(ProgramAST* ast, CregDecl* creg);

// Debug functions
void print_ast(ProgramAST* ast);
void free_ast(ProgramAST* ast);
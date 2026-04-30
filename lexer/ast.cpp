#include "ast.hpp"
#include<bits/stdc++.h>

using namespace std;

GateNode* create_gate_node(GateType type, Qubit* target, Qubit* control) {
    GateNode* node = new GateNode();
    if (!node) {
        cerr << "Error: Memory allocation failed for gate node" << endl;
        return nullptr;
    }
    node->type = type;
    node->target = target;
    node->control = control;
    node->classical_bit = nullptr;
    node->next = nullptr;
    return node;
}

GateNode* create_measure_node(Qubit* target, string* classical_bit) {
    GateNode* node = new GateNode();
    if (!node) {
        cout<< "Error: Memory allocation failed for measure node" << endl;
        return nullptr;
    }
    node->type = GateType::MEASURE;
    node->target = target;
    node->control = nullptr;
    node->classical_bit = classical_bit;
    node->next = nullptr;
    return node;
}

Qubit* create_qubit(string* reg_name, int index) {
    Qubit* q = new Qubit();
    if (!q) {
        cout << "Error: Memory allocation failed for qubit" << endl;
        return nullptr;
    }
    q->reg_name = reg_name;
    q->index = index;
    return q;
}

QregDecl* create_qreg_decl(string* name, int size) {
    QregDecl* qreg = new QregDecl();
    if (!qreg) {
        cout << "Error: Memory allocation failed for qreg" << endl;
        return nullptr;
    }
    qreg->name = name;
    qreg->size = size;
    qreg->next = nullptr;
    return qreg;
}

CregDecl* create_creg_decl(string* name, int size) {
    CregDecl* creg = new CregDecl();
    if (!creg) {
        cout << "Error: Memory allocation failed for creg" << endl;
        return nullptr;
    }
    creg->name = name;
    creg->size = size;
    creg->next = nullptr;
    return creg;
}

ProgramAST* create_program_ast(void) {
    ProgramAST* ast = new ProgramAST();
    if (!ast) {
        cerr << "Error: Memory allocation failed for program AST" << endl;
        return nullptr;
    }
    ast->qubit_registers = nullptr;
    ast->classical_registers = nullptr;
    ast->gates = nullptr;
    return ast;
}

void add_gate_to_program(ProgramAST* ast, GateNode* gate) {
    if(ast->gates==nullptr) ast->gates = gate;
    else {
            GateNode *  current= ast->gates;
            while(current->next){
                current = current->next;
            }
            current->next = gate;

    }
}

void add_qreg_to_program(ProgramAST* ast, QregDecl* qreg) {
    if(ast->qubit_registers==nullptr) ast->qubit_registers = qreg;
    else {
            QregDecl *  current= ast->qubit_registers;
            while(current->next){
                current = current->next;
            }
            current->next = qreg;

    }
}

void add_creg_to_program(ProgramAST* ast, CregDecl* creg) {
    if(ast->classical_registers==nullptr) ast->classical_registers = creg;
    else {
            CregDecl *  current= ast->classical_registers;
            while(current->next){
                current = current->next;
            }
            current->next = creg;

    }
}

static const char* gate_type_to_string(GateType type) {
    switch(type) {
        case GateType::HADAMARD: return "H";
        case GateType::PAULI_X: return "X";
        case GateType::PAULI_Z: return "Z";
        case GateType::CX: return "CX";
        case GateType::MEASURE: return "MEASURE";
        case GateType::GATE_BARRIER: return "BARRIER";
        default: return "UNKNOWN";
    }
}

void print_ast(ProgramAST* ast) {
    if (!ast) {
        cout << "AST is NULL" << endl;
        return;
    }
    
    cout << "\n=== AST ===" << endl;
    
    // Print qubit registers
    QregDecl* q = ast->qubit_registers;
    while (q) {
        cout << "QREG: " << *(q->name) << "[" << q->size << "]" << endl;
        q = q->next;
    }
    
    // Print classical registers
    CregDecl* c = ast->classical_registers;
    while (c) {
        cout << "CREG: " << *(c->name) << "[" << c->size << "]" << endl;
        c = c->next;
    }
    
    // Print gates
    GateNode* g = ast->gates;
    int gate_num = 1;
    while (g) {
        cout << "Gate " << gate_num++ << ": " << gate_type_to_string(g->type) << " ";
        
        if (g->type == GateType::CX) {
            cout << "control=" << *(g->control->reg_name) << "[" << g->control->index << "], "
                 << "target=" << *(g->target->reg_name) << "[" << g->target->index << "]";
        } else if (g->type == GateType::MEASURE) {
            cout << *(g->target->reg_name) << "[" << g->target->index << "] -> " << *(g->classical_bit);
        } else {
            cout << *(g->target->reg_name) << "[" << g->target->index << "]";
        }
        cout << endl;
        g = g->next;
    }
    cout << "==========" << endl << endl;
}

void free_ast(ProgramAST* ast) {
    if (!ast) return;
    
    // Free qubit registers
    QregDecl* q = ast->qubit_registers;
    while (q) {
        QregDecl* next = q->next;
        delete q->name;
        delete q;
        q = next;
    }
    
    // Free classical registers
    CregDecl* c = ast->classical_registers;
    while (c) {
        CregDecl* next = c->next;
        delete c->name;
        delete c;
        c = next;
    }
    
    // Free gates
    GateNode* g = ast->gates;
    while (g) {
        GateNode* next = g->next;
        if (g->target) {
            delete g->target->reg_name;
            delete g->target;
        }
        if (g->control) {
            delete g->control->reg_name;
            delete g->control;
        }
        if (g->classical_bit) {
            delete g->classical_bit;
        }
        delete g;
        g = next;
    }
    
    delete ast;
}
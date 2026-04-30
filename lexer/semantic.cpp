#include "semantic.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace std;

SymbolTable* create_symbol_table(void) {
    SymbolTable* st = new SymbolTable();
    if (!st) return nullptr;
    st->qubit_regs = nullptr;
    st->num_qubit_regs = 0;
    return st;
}

void free_symbol_table(SymbolTable* st) {
    if (!st) return;
    for (int i = 0; i < st->num_qubit_regs; i++) {
        delete st->qubit_regs[i].name; 
    }
    delete[] st->qubit_regs;
    delete st;
}

void add_qubit_register(SymbolTable* st, string* name, int size) {
    st->num_qubit_regs++;
    RegisterInfo* new_regs = new RegisterInfo[st->num_qubit_regs];
    
    // Copy old data
    for (int i = 0; i < st->num_qubit_regs - 1; i++) {
        new_regs[i] = st->qubit_regs[i];
    }
    
    // Add new register
    new_regs[st->num_qubit_regs - 1].name = new string(*name);
    new_regs[st->num_qubit_regs - 1].size = size;
    
    // Clean up old array
    delete[] st->qubit_regs;
    st->qubit_regs = new_regs;
}



int validate_qubit(SymbolTable* st, string* reg_name, int index) {
    for (int i = 0; i < st->num_qubit_regs; i++) {
        if (*(st->qubit_regs[i].name) == *reg_name) {
            if (index < 0 || index >= st->qubit_regs[i].size) {
                cerr << "Error: Qubit index " << index 
                     << " out of range for register " << *reg_name 
                     << " (size " << st->qubit_regs[i].size << ")" << endl;
                return 0;
            }
            return 1;
        }
    }
    cerr << "Error: Unknown qubit register: " << *reg_name << endl;
    return 0;
}

int semantic_validate(ProgramAST* ast) {
    if (!ast) {
        cerr << "Error: AST is NULL" << endl;
        return 0;
    }
    
    SymbolTable* st = create_symbol_table();
    int valid = 1;
    
    // First pass: collect all register declarations
    QregDecl* q = ast->qubit_registers;
    while (q) {
        add_qubit_register(st, q->name, q->size);
        q = q->next;
    }
    

    
    // Second pass: validate all gate operations
    GateNode* g = ast->gates;
    while (g && valid) {
        if (g->type == GateType::MEASURE) {
            // Validate target qubit
            if (g->target) {
                valid = validate_qubit(st, g->target->reg_name, g->target->index);
            }
        } else if (g->type == GateType::CX) {
            // Validate control and target qubits
            if (g->control) {
                valid = validate_qubit(st, g->control->reg_name, g->control->index);
            }
            if (g->target && valid) {
                valid = validate_qubit(st, g->target->reg_name, g->target->index);
            }
        } else if (g->type != GateType::GATE_BARRIER) {
            // Single qubit gate
            if (g->target) {
                valid = validate_qubit(st, g->target->reg_name, g->target->index);
            }
        }
        g = g->next;
    }
    
    free_symbol_table(st);
    return valid;
}

void print_validation_report(ProgramAST* ast, int passed) {
    cout << "\n=== Semantic Validation Report ===" << endl;
    
    if (passed) {
        cout << "Status: ✓ PASSED" << endl;
        
        // Print register summary
        cout << "\nRegisters:" << endl;
        QregDecl* q = ast->qubit_registers;
        while (q) {
            cout << "  Qubit: " << *(q->name) << "[" << q->size << "]" << endl;
            q = q->next;
        }
        CregDecl* c = ast->classical_registers;
        while (c) {
            cout << "  Classical: " << *(c->name) << "[" << c->size << "]" << endl;
            c = c->next;
        }
        
        
        int num_gates = 0;
        GateNode* g = ast->gates;
        while (g) {
            num_gates++;
            g = g->next;
        }
        cout << "\nTotal gates: " << num_gates << endl;
        
    } else {
        cout << "Status: ✗ FAILED" << endl;
        cout << "Please fix errors and re-run" << endl;
    }
    cout << "================================" << endl << endl;
}
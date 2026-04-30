#pragma once

#include "ast.hpp"
#include <string>

using namespace std;

typedef struct {
    string* name;
    int size;
} RegisterInfo;

typedef struct {
    RegisterInfo* qubit_regs;
    int num_qubit_regs;
} SymbolTable;


SymbolTable* create_symbol_table(void);
void free_symbol_table(SymbolTable* st);
void add_qubit_register(SymbolTable* st, string* name, int size);
int validate_qubit(SymbolTable* st, string* reg_name, int index);

int semantic_validate(ProgramAST* ast);
void print_validation_report(ProgramAST* ast, int passed);
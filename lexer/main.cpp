#include "ast.hpp"
#include "semantic.hpp"
#include <iostream>
#include <cstdio>
#include <getopt.h>

using namespace std;

extern FILE* yyin;
extern int yyparse();
extern ProgramAST* g_ast;

void print_usage(const char* prog_name) {
    cout << "Usage: " << prog_name << " <input.qasm> [options]\n";
    cout << "Options:\n";
    cout << "  --dump-ast             Dump AST before generation\n";
    cout << "  -h, --help             Show this help\n";
}

int main(int argc, char** argv) {
    const char* input_file = nullptr;
    bool dump_ast = false;

    static struct option long_options[] = {
        {"dump-ast", no_argument, 0, 1},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 1:
                dump_ast = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (optind < argc) {
        input_file = argv[optind];
    } else {
        print_usage(argv[0]);
        return 1;
    }
    
    yyin = fopen(input_file, "r");
    if (!yyin) {
        cerr << "ERROR: Cannot open file " << input_file << endl;
        return 1;
    }
    
    cout << "Parsing: " << input_file << endl;
    
    if (yyparse() != 0) {
        cerr << "ERROR: Parsing failed" << endl;
        fclose(yyin);
        return 1;
    }
    fclose(yyin);
    
    if (!semantic_validate(g_ast)) {
        cerr << "✗ Phase 1 failed" << endl;
        free_ast(g_ast);
        return 1;
    }
    
    if (dump_ast) {
        print_ast(g_ast);
    }
    
    cout << "\n✓ Phase 1 completed successfully" << endl;
    free_ast(g_ast);
    return 0;
}
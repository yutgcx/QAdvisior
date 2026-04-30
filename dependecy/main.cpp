#include "../lexer/ast.hpp"
#include "../lexer/semantic.hpp"
#include "dependency.hpp"
#include "scheduler.hpp"
#include "analysis.hpp"

extern FILE* yyin;
extern int yyparse();
extern ProgramAST* g_ast;

#include <iostream>
#include <string>
#include <cstring>
#include <getopt.h>

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " <input.qasm> [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --dump-ast             Dump AST before generation\n";
    std::cout << "  -h, --help             Show this help\n";
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
    if (!yyin) return 1;
    if (yyparse() != 0) return 1;
    fclose(yyin);
    
    if (!g_ast || !semantic_validate(g_ast)) return 1;
    
    if (dump_ast) {
        print_ast(g_ast);
    }
    
    int num_gates;
    GateInfo** gates = extract_gate_info(g_ast, &num_gates);
    if (!gates || num_gates == 0) return 1;
    
    DependencyGraph* dep_graph = create_dependency_graph(num_gates);
    dep_graph->gates = gates;
    build_dependencies(dep_graph);
    
    print_dependency_graph(dep_graph);
    
    Schedule* schedule = compute_parallel_schedule(dep_graph);
    if (!schedule) return 1;
    
    print_schedule(schedule, dep_graph);
    
    AnalysisMetrics* metrics = compute_metrics(dep_graph, schedule);
    print_analysis_report(dep_graph, schedule, metrics);
    
    free_analysis_metrics(metrics);
    free_schedule(schedule);
    free_dependency_graph(dep_graph);
    free_ast(g_ast);
    
    return 0;
}

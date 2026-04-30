#include "qir_generator.hpp"

// Include Phase 1 and Phase 2 headers
#include "../lexer/ast.hpp"
#include "../lexer/semantic.hpp"
#include "../dependecy/dependency.hpp"
#include "../dependecy/scheduler.hpp"
#include "../dependecy/analysis.hpp"


// External declarations from Flex/Bison
extern FILE* yyin;
extern int yyparse();
extern ProgramAST* g_ast;

#include <iostream>
#include <string>
#include <cstring>
#include <getopt.h>

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " <input.qasm> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o, --output <file>    Output LLVM IR file\n";
    std::cout << "  -O, --optimize <level> Optimization level (0,1,2,3) [default: 2]\n";
    std::cout << "  --dump-ast             Dump AST before generation\n";
    std::cout << "  --dump-deps            Dump dependency graph\n";
    std::cout << "  --dump-schedule        Dump parallel schedule\n";
    std::cout << "  --verify               Verify generated IR\n";
    std::cout << "  -h, --help             Show this help\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << prog_name << " bell.qasm -o bell.ll\n";
    std::cout << "  " << prog_name << " ghz.qasm -O3 --verify\n";
}

int main(int argc, char** argv) {
    // Parse command line arguments
    const char* input_file = nullptr;
    const char* output_file = nullptr;
    int opt_level = 2;
    bool dump_ast = false;
    bool dump_deps = false;
    bool dump_schedule = false;
    bool verify_ir = false;
    
    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"optimize", required_argument, 0, 'O'},
        {"dump-ast", no_argument, 0, 1},
        {"dump-deps", no_argument, 0, 2},
        {"dump-schedule", no_argument, 0, 3},
        {"verify", no_argument, 0, 4},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "o:O:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'o':
                output_file = optarg;
                break;
            case 'O':
                opt_level = atoi(optarg);
                if (opt_level < 0 || opt_level > 3) opt_level = 2;
                break;
            case 1:
                dump_ast = true;
                break;
            case 2:
                dump_deps = true;
                break;
            case 3:
                dump_schedule = true;
                break;
            case 4:
                verify_ir = true;
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
    
    // ========== PHASE 1: Parse ==========
    std::cout << "Phase 1: Parsing " << input_file << "...\n";
    
    yyin = fopen(input_file, "r");
    if (!yyin) {
        std::cerr << "Error: Cannot open file " << input_file << std::endl;
        return 1;
    }
    
    if (yyparse() != 0) {
        std::cerr << "Error: Parsing failed\n";
        fclose(yyin);
        return 1;
    }
    fclose(yyin);
    
    if (!g_ast) {
        std::cerr << "Error: AST is NULL\n";
        return 1;
    }
    
    // Semantic validation
    if (!semantic_validate(g_ast)) {
        std::cerr << "Error: Semantic validation failed\n";
        return 1;
    }
    
    if (dump_ast) {
        print_ast(g_ast);
    }
    
    // ========== PHASE 2: Dependency Analysis ==========
    std::cout << "Phase 2: Analyzing dependencies...\n";
    
    int num_gates;
    GateInfo** gates = extract_gate_info(g_ast, &num_gates);
    if (!gates || num_gates == 0) {
        std::cerr << "Error: No gates found\n";
        return 1;
    }
    
    DependencyGraph* dep_graph = create_dependency_graph(num_gates);
    dep_graph->gates = gates;
    build_dependencies(dep_graph);
    
    if (dump_deps) {
        print_dependency_graph(dep_graph);
    }
    
    // Compute parallel schedule
    Schedule* schedule = compute_parallel_schedule(dep_graph);
    if (!schedule) {
        std::cerr << "Error: Failed to compute schedule\n";
        return 1;
    }
    
    if (dump_schedule) {
        print_schedule(schedule, dep_graph);
    }
    
    // Compute metrics
    AnalysisMetrics* metrics = compute_metrics(dep_graph, schedule);
    print_analysis_report(dep_graph, schedule, metrics);
    
    // ========== PHASE 3: QIR Generation ==========
    std::cout << "Phase 3: Generating QIR...\n";
    
    qadvisor::QIRGenerator generator("quantum_circuit", opt_level);
    
    if (!generator.generate(g_ast, schedule, dep_graph)) {
        std::cerr << "Error: QIR generation failed\n";
        return 1;
    }
    
    if (verify_ir) {
        if (generator.verify()) {
            std::cout << "✓ IR verification passed\n";
        } else {
            std::cerr << "✗ IR verification failed\n";
            return 1;
        }
    }
    
    // Output IR
    if (output_file) {
        generator.writeToFile(output_file);
        std::cout << "Generated QIR written to: " << output_file << std::endl;
    } else {
        generator.writeToStdOut();
    }
    
    // ========== Cleanup ==========
    free_analysis_metrics(metrics);
    free_schedule(schedule);
    free_dependency_graph(dep_graph);
    free_ast(g_ast);
    
    std::cout << "\n✓ Phase 3 completed successfully\n";
    
    return 0;
}
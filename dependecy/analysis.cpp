#include "analysis.hpp"
#include <iostream>
#include <cstdlib>

using namespace std;

AnalysisMetrics* compute_metrics(DependencyGraph* graph, Schedule* schedule) {
    AnalysisMetrics* m = new AnalysisMetrics();
    m->original_depth = graph ? graph->num_gates : 0;
    m->parallel_depth = schedule ? schedule->num_groups : 0;
    m->speedup = schedule ? schedule->speedup : 1.0f;
    return m;
}

void print_analysis_report(DependencyGraph* graph, Schedule* schedule, AnalysisMetrics* metrics) {
    cout << "=== Execution Analysis ===" << endl;
    if (metrics) {
        cout << "Original Depth: " << metrics->original_depth << endl;
        cout << "Parallel Depth: " << metrics->parallel_depth << endl;
        cout << "Speedup:        " << metrics->speedup << "x" << endl;
    }
}

void free_analysis_metrics(AnalysisMetrics* metrics) {
    delete metrics;
}

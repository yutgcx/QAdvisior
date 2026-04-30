#pragma once
#include "dependency.hpp"
#include "scheduler.hpp"

struct AnalysisMetrics {
    int original_depth;
    int parallel_depth;
    float speedup;
};

AnalysisMetrics* compute_metrics(DependencyGraph* graph, Schedule* schedule);
void print_analysis_report(DependencyGraph* graph, Schedule* schedule, AnalysisMetrics* metrics);
void free_analysis_metrics(AnalysisMetrics* metrics);

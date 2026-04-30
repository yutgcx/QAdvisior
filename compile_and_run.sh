#!/bin/bash

# Set default input if not provided
INPUT_FILE=${1:-input.qasm}
OUTPUT_FILE="output.ll"

echo "================================================="
echo "QAdvisor Compiler Pipeline (Phase 1 -> 2 -> 3)"
echo "================================================="

# Ensure the executable exists
if [ ! -f "./build/qadvisor_phase3" ]; then
    echo "Compiler binary not found! Building now..."
    cmake --build build --target qadvisor_phase3
fi

# Step 1: Run through our phases
echo -e "\n[1] Processing ${INPUT_FILE}..."
./build/qadvisor_phase3 "$INPUT_FILE" -o "$OUTPUT_FILE"

if [ $? -eq 0 ]; then
    echo -e "\n[2] Summary:"
    echo "✓ Successfully passed Phase 1 (Lexer/Parser)"
    echo "✓ Successfully passed Phase 2 (Dependency/Scheduler)"
    echo "✓ Successfully passed Phase 3 (LLVM QIR IR generation)"
    echo -e "\nGenerated QIR artifact: ${OUTPUT_FILE}"
    echo "-------------------------------------------------"
    echo "Executing Python main.py ..."
    OUTPUT_ABS=$(realpath "$OUTPUT_FILE")
    cd qir-executor && python3 main.py "$OUTPUT_ABS"
else
    echo -e "\n✗ Compilation pipeline failed. Please check the syntax of your .qasm code."
    exit 1
fi

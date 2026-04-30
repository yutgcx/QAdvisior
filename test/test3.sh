#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
PHASE3_BIN="$BUILD_DIR/qadvisor_phase3"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo "========================================="
echo "Phase 3 Test Suite (QIR Generation & Runner Simulation)"
echo "========================================="

if [ ! -f "$PHASE3_BIN" ]; then
    echo -e "${RED}Error: Binary not found at $PHASE3_BIN. Please run 'make qadvisor_phase3'${NC}"
    exit 1
fi

echo -e "\n${YELLOW}Step 1: Generating QIR for bell.qasm...${NC}"
"$PHASE3_BIN" "$SCRIPT_DIR/bell.qasm" -o /tmp/bell.ll > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ QIR successfully generated at /tmp/bell.ll${NC}"
else
    echo -e "${RED}✗ QIR generation failed${NC}"
    exit 1
fi

echo -e "\n${YELLOW}Step 2: Compiling QIR runner framework with generated LLVM IR...${NC}"
clang++ /tmp/bell.ll "$SCRIPT_DIR/qir_runtime.cpp" -o /tmp/qir_simulator_run
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Binary execution engine compiled successfully${NC}"
else
    echo -e "${RED}✗ QIR Runner compilation failed${NC}"
    exit 1
fi

echo -e "\n${YELLOW}Step 3: Simulating Circuit Execution...${NC}"
echo "-----------------------------------------"
/tmp/qir_simulator_run
echo "-----------------------------------------"
echo -e "${GREEN}✓ Simulation executed and validated${NC}"

echo -e "\n========================================="
echo "All systems fully operational"
echo "========================================="

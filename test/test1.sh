#!/bin/bash

# Move to script directory (important!)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Build directory (CMake default)
BUILD_DIR="$SCRIPT_DIR/../build"

# Binary name (adjust if different in CMakeLists.txt)
PHASE1_BIN="$BUILD_DIR/qadvisor_phase1"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo "========================================="
echo "Phase 1 Test Suite"
echo "========================================="

# Check if binary exists
if [ ! -f "$PHASE1_BIN" ]; then
    echo -e "${RED}Error: Binary not found at $PHASE1_BIN${NC}"
    echo "Run: cmake -B build && cmake --build build"
    exit 1
fi

# Test 1: Valid Bell circuit
echo -e "\n${YELLOW}Test 1: Bell circuit${NC}"
cat > /tmp/test1.qasm << 'EOF'
qreg q[2];
creg c[2];
h q[6];
cx q[0], q[1];
measure q[0] -> c[0];
measure q[1] -> c[1];
EOF

"$PHASE1_BIN" /tmp/test1.qasm --dump-ast > /tmp/test1.out 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Test 1 passed${NC}"
else
    echo -e "${RED}✗ Test 1 failed${NC}"
    cat /tmp/test1.out
fi

# Test 2: Invalid syntax
echo -e "\n${YELLOW}Test 2: Invalid syntax (missing semicolon)${NC}"
cat > /tmp/test2.qasm << 'EOF'

qreg q[2]
h q[0];
EOF

"$PHASE1_BIN" /tmp/test2.qasm 2> /tmp/test2.err
if [ $? -ne 0 ]; then
    echo -e "${GREEN}✓ Test 2 passed (caught syntax error)${NC}"
else
    echo -e "${RED}✗ Test 2 failed (should have errored)${NC}"
fi

# Test 3: Semantic error (out-of-range qubit)
echo -e "\n${YELLOW}Test 3: Semantic error (out of range qubit)${NC}"
cat > /tmp/test3.qasm << 'EOF'
OPENQASM 2.0;
include "qelib1.inc";
qreg q[2];
h q[5];
EOF

"$PHASE1_BIN" /tmp/test3.qasm 2> /tmp/test3.err
if [ $? -ne 0 ]; then
    echo -e "${GREEN}✓ Test 3 passed (caught semantic error)${NC}"
else
    echo -e "${RED}✗ Test 3 failed (should have errored)${NC}"
fi

echo -e "\n${YELLOW}Test 4: Lexer custom test (Real No. and Strings)${NC}"
cat > /tmp/test4.qasm << 'EOF'
OPENQASM 2.0;
include "my_lib.inc";
qreg q[1];
creg c[1];
EOF

"$PHASE1_BIN" /tmp/test4.qasm --validate-only 2> /tmp/test4.err
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Test 4 passed (Lexer successfully scanned OPENQASM and include)${NC}"
else
    echo -e "${RED}✗ Test 4 failed${NC}"
fi


echo -e "\n${YELLOW}Test 5: Testing working of Phase 1 with bell.qasm${NC}"
"$PHASE1_BIN" "$SCRIPT_DIR/bell.qasm" --dump-ast > /tmp/bell_phase1.out 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Test 5 passed (AST generated for bell.qasm)${NC}"
else
    echo -e "${RED}✗ Test 5 failed${NC}"
fi

echo -e "\n========================================="
echo "All tests completed"
echo "========================================="
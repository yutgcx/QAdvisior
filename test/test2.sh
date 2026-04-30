#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
PHASE2_BIN="$BUILD_DIR/qadvisor_phase2"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo "========================================="
echo "Phase 2 Test Suite (Dependency & Schedule)"
echo "========================================="

if [ ! -f "$PHASE2_BIN" ]; then
    echo -e "${RED}Error: Binary not found at $PHASE2_BIN${NC}"
    exit 1
fi

echo -e "\n${YELLOW}Test 1: Bell circuit Phase 2${NC}"
"$PHASE2_BIN" "$SCRIPT_DIR/bell.qasm" > /tmp/bell_phase2.out 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Test 1 passed${NC}"
else
    echo -e "${RED}✗ Test 1 failed${NC}"
    cat /tmp/bell_phase2.out
fi

echo -e "\n${YELLOW}Test 2: GHZ circuit Phase 2${NC}"
"$PHASE2_BIN" "$SCRIPT_DIR/ghz.qasm" > /tmp/ghz_phase2.out 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Test 2 passed${NC}"
else
    echo -e "${RED}✗ Test 2 failed${NC}"
    cat /tmp/ghz_phase2.out
fi

echo -e "\n${YELLOW}Test 3: Custom parallel scheduling circuit Phase 2${NC}"
cat > /tmp/test_parallel.qasm << 'EOF'
qreg q[4];
h q[0];
h q[1];
h q[2];
h q[3];
EOF

"$PHASE2_BIN" /tmp/test_parallel.qasm > /tmp/parallel_phase2.out 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Test 3 passed (Successfully analyzed parallel circuit)${NC}"
    # Extract speedup
    grep "Speedup" /tmp/parallel_phase2.out
else
    echo -e "${RED}✗ Test 3 failed${NC}"
    cat /tmp/parallel_phase2.out
fi

echo -e "\n========================================="
echo "All tests completed"
echo "========================================="

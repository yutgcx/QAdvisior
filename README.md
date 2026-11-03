# QAdvisor: Technical Architecture, Feature Analysis & Innovation Roadmap

---

## 1. Executive Summary

**QAdvisor** is an end-to-end quantum compilation, optimization, and execution framework. It ingests quantum circuits written in **OpenQASM (Quantum Assembly Language)**, analyzes inter-gate qubit dependencies to schedule operations into parallel execution cycles, lowers the circuit into Microsoft's **Quantum Intermediate Representation (QIR)** based on LLVM, and verifies correctness using an integrated statevector simulation runtime.

### Key Objectives
* **Circuit Depth Reduction:** Minimize gate latency and critical path depth through automated dependency DAG analysis and greedy parallel scheduling.
* **Standards-Compliant QIR Generation:** Produce target-agnostic LLVM IR adhering to Microsoft Quantum Intermediate Representation (QIR) specifications.
* **End-to-End Simulation & Verification:** Bridge the compiled QIR to native machine execution via LLVM/Clang and statevector measurement sampling.

---

## 2. System Architecture & Pipeline

The toolchain operates across four distinct phases:

```
                  ┌───────────────────────────────┐
                  │    OpenQASM Input Circuit     │
                  └───────────────┬───────────────┘
                                  │
                                  ▼
┌──────────────────────────────────────────────────────────────────┐
│ Phase 1: Lexing, Parsing & Semantic Validation                   │
│   • Flex Lexer (qasm.l) & Bison Parser (qasm.ypp)                │
│   • Program AST construction (ProgramAST, GateNode, QregDecl)    │
│   • Semantic analysis (semantic.cpp): register & index bounds    │
└─────────────────────────────────┬────────────────────────────────┘
                                  │
                                  ▼
┌──────────────────────────────────────────────────────────────────┐
│ Phase 2: Dependency Graph & Parallel Scheduler                   │
│   • Gate dependency extraction (dependency.cpp)                  │
│   • Qubit & classical bit overlap tracking (RAW/WAR hazards)     │
│   • Greedy topological cycle scheduler (scheduler.cpp)           │
│   • Theoretical speedup & depth analysis (analysis.cpp)          │
└─────────────────────────────────┬────────────────────────────────┘
                                  │
                                  ▼
┌──────────────────────────────────────────────────────────────────┐
│ Phase 3: LLVM QIR Code Generator                                 │
│   • TypeManager & QubitManager pointer abstraction               │
│   • QIR intrinsic emission (__quantum__qis__*)                   │
│   • Metadata generation (required_num_qubits, entry_point)       │
│   • LLVM optimization passes (InstCombine, GVN, SimplifyCFG)     │
└─────────────────────────────────┬────────────────────────────────┘
                                  │
                                  ▼
┌──────────────────────────────────────────────────────────────────┐
│ Phase 4: Execution & Simulation Runtime                          │
│   • C ABI runtime stubs (test/qir_runtime.cpp)                   │
│   • Native statevector simulator (qbraid/runtime/native.py)      │
│   • Multi-shot measurement sampling & probability distribution   │
└──────────────────────────────────────────────────────────────────┘
```

---

## 3. Detailed Component Breakdown

### Phase 1: Lexical Analysis, Parsing & Semantics
* **Files:** `lexer/qasm.l`, `lexer/qasm.ypp`, `lexer/ast.hpp`, `lexer/ast.cpp`, `lexer/semantic.hpp`, `lexer/semantic.cpp`, `lexer/main.cpp`
* **Binary Target:** `qadvisor_phase1`
* **Functionality:**
  * Tokenizes OpenQASM source, extracting quantum registers (`qreg`), classical registers (`creg`), and quantum gates.
  * Builds an Abstract Syntax Tree (AST) representing declarations and gate sequences.
  * Validates semantic constraints:
    * Rejects duplicate register names.
    * Ensures qubit indices are within allocated register bounds.
    * Disallows identical control and target qubits on two-qubit gates (e.g., `cx q[0], q[0]`).
* **Supported Gate Primitives:**
  * Single-Qubit: Hadamard (`h`), Pauli-X (`x`), Pauli-Z (`z`)
  * Two-Qubit: Controlled-NOT (`cx`)
  * Measurement: `measure q[i] -> c[i]`

### Phase 2: Dependency Analysis & Parallel Scheduling
* **Files:** `dependecy/dependency.hpp`, `dependecy/dependency.cpp`, `dependecy/scheduler.hpp`, `dependecy/scheduler.cpp`, `dependecy/analysis.hpp`, `dependecy/analysis.cpp`, `dependecy/main.cpp`
* **Binary Target:** `qadvisor_phase2`
* **Functionality:**
  * **Directed Acyclic Graph (DAG):** Constructs a gate dependency graph where each gate is a node and directed edges represent qubit or classical bit data hazards.
  * **Parallel Gate Scheduling:** Applies a greedy topological scheduling algorithm to group non-interfering gates into parallel execution cycles (`ParallelGroup`).
  * **GraphViz DOT Export:** Emits `.dot` graphs for visual graph inspection.
  * **Performance Metrics:** Evaluates sequential cycle depth ($D_{\text{seq}}$) versus parallel cycle depth ($D_{\text{par}}$) and reports speedup:
    $$\text{Speedup} = \frac{D_{\text{seq}}}{D_{\text{par}}}$$

### Phase 3: LLVM QIR Generation
* **Files:** `qir/qir_generator.hpp`, `qir/qir_generator.cpp`, `qir/qubit_manager.hpp`, `qir/qubit_manager.cpp`, `qir/type_manager.hpp`, `qir/type_manager.cpp`, `qir/metadata_builder.hpp`, `qir/metadata_builder.cpp`, `qir/main.cpp`
* **Binary Target:** `qadvisor_phase3`
* **Functionality:**
  * **QIR Specification Compliance:** Generates LLVM IR compatible with Microsoft QIR specification using opaque pointer notation.
  * **Runtime Declarations:**
    * `declare void @__quantum__qis__h__body(ptr)`
    * `declare void @__quantum__qis__x__body(ptr)`
    * `declare void @__quantum__qis__z__body(ptr)`
    * `declare void @__quantum__qis__cnot__body(ptr, ptr)`
    * `declare void @__quantum__qis__mz__body(ptr, ptr)`
  * **Qubit Pointer Mapping:** Maps discrete qubit IDs to deterministic pointer indices via `inttoptr (i64 <id> to ptr)`.
  * **Metadata Annotation:** Generates module-level flags and entry-point metadata (`qir_profiles`, `required_num_qubits`, `required_num_results`).
  * **LLVM Optimizations:** Configurable optimization levels (`-O0` to `-O3`) invoking passes like `InstCombine`, `GVN`, and `SimplifyCFG`.

### Phase 4: Execution Engine & Statevector Simulator
* **Files:** `compile_and_run.sh`, `test/qir_runtime.cpp`, `qir-executor/main.py`, `qir-executor/qbraid/runtime/native.py`
* **Functionality:**
  * **Dynamic JIT Compilation:** Compiles the generated LLVM IR (`output.ll`) alongside C++ runtime stubs using `clang++`.
  * **Statevector Simulation:** Tracks the exact $2^N$-dimensional complex amplitude statevector:
    * Simulates $H, X, Z,$ and $CX$ matrix operations.
    * Computes bitstring measurement probabilities: $P(|x\rangle) = |\alpha_x|^2$.
    * Draws stochastic samples over a configurable number of shots (default: 1024).
  * **Output Generation:** Exports measurement counts and probability distributions to `results.json` and terminal summaries.

---

## 4. Current Limitations & Technical Debt

1. **Gate Set Expressiveness:** Currently restricted to Clifford+$Z$ primitives. Lacks arbitrary single-qubit rotations ($R_x(\theta), R_y(\theta), R_z(\theta), U_3$), phase gates ($S, T$), and multi-qubit operations ($SWAP, CCX$).
2. **All-to-All Connectivity Assumption:** Phase 2 assumes unconstrained qubit interaction. Real quantum processors (superconducting, neutral atom) possess fixed coupling topologies (heavy-hex, grid).
3. **QASM Grammar Gaps:** Statements such as `OPENQASM 2.0;` and `include "qelib1.inc";` are recognized by the lexer but lack corresponding grammar productions in `qasm.ypp`.
4. **Static Circuit Model:** Does not yet support dynamic mid-circuit measurement feedback (`if (c == 1)`) lowered into LLVM control flow (`br i1`).
5. **Build Configuration:** `CMakeLists.txt` links specifically to `LLVM-22` rather than resolving dynamic system library definitions (`${LLVM_LIBRARIES}`).

---

## 5. Innovative Implementation Roadmap

To transition QAdvisor from a functional prototype into an industry-grade, differentiated quantum platform, the following six architectural enhancements are recommended:

### 1. Intelligent Hardware Selection & Advisory Engine
* **Concept:** Fulfill the promise of the name **"QAdvisor"** by providing comparative backend recommendations.
* **Mechanism:**
  * Maintain hardware profiles for major architectures (IBM Eagle/Heron, Rigetti Ankaa, IonQ Forte, Quantinuum H-series).
  * Calculate an **Estimated Success Probability (ESP)** for the circuit on each machine:
    $$\text{ESP} \approx \prod_{g \in 1Q} (1 - \epsilon_{1q}) \times \prod_{g \in 2Q} (1 - \epsilon_{2q}) \times \exp\left(-\frac{t_{\text{circuit}}}{T_1}\right)$$
  * Deliver actionable compiler diagnostics (e.g., *"Recommended Target: IonQ Forte (estimated fidelity: 97.4%, 0 SWAP overhead) vs. IBM Heron (estimated fidelity: 82.1%, 6 SWAPs required)"*).

### 2. Topology-Aware Routing & Noise-Adaptive Qubit Allocation
* **Concept:** Eliminate the all-to-all connectivity assumption using physical topology constraints.
* **Mechanism:**
  * Implement the **SABRE** (SWAP-based Bidirectional heuristic Search) routing pass.
  * Integrate live or simulated QPU calibration graphs (qubit $T_1, T_2$ times, readout fidelity, two-qubit error rates).
  * Formulate an A* or heuristic mapping pass that assigns frequently interacting circuit qubits to physical qubits exhibiting the highest fidelity and coherence times.

### 3. Quantum Circuit Optimization Passes
* **Concept:** Implement circuit-level simplification before generating QIR.
* **Passes:**
  * **Peephole Cancellation:** Detect and prune inverse pairs ($H \cdot H = I$, $X \cdot X = I$, $CX_{a,b} \cdot CX_{a,b} = I$).
  * **Rotation Merging:** Combine sequential single-axis rotations: $R_z(\theta_1) R_z(\theta_2) \to R_z(\theta_1 + \theta_2)$.
  * **Commutation Reordering:** Exploit known commutation relations ($Z$ gates commute through control qubits of $CX$; $X$ gates commute through target qubits) to bubble independent gates earlier in the schedule, unlocking higher parallel speedup.

### 4. Dynamic Circuits & Adaptive QIR Profile
* **Concept:** Capitalize on LLVM's primary advantage over static quantum DAGs: native control-flow representation.
* **Mechanism:**
  * Support mid-circuit measurements with classical conditional branching:
    ```qasm
    measure q[0] -> c[0];
    if (c[0] == 1) x q[1];
    ```
  * Lower classical conditioning directly into LLVM basic blocks (`br i1 %cond, label %then, label %else`) invoking `@__quantum__qis__read_result__body`.
  * Enables Quantum Error Correction (QEC) syndrome decoding and quantum teleportation protocols directly in QIR.

### 5. Interactive Circuit Profiling & Timeline Visualizer
* **Concept:** Visual representation of circuit execution cycles and resource bottlenecks.
* **Deliverables:**
  * **Gantt / Timeline View:** Render an interactive terminal or HTML visualizer mapping each qubit's activity per cycle.
  * **Critical Path & Idle Heatmap:** Highlight idle periods where qubits are vulnerable to decoherence ($T_1/T_2$ decay), motivating dynamical decoupling insertion.

### 6. Quantum Error Mitigation (QEM) Synthesizer
* **Concept:** Automatically inject error mitigation techniques during QIR compilation.
* **Techniques:**
  * **Dynamical Decoupling (DD):** Insert $XY4$ or CPMG pulse trains on idle qubit cycles identified by the Phase 2 scheduler.
  * **Zero-Noise Extrapolation (ZNE):** Emit scaled QIR variants with folded unitary gates ($U \to U U^\dagger U$) to enable noise-extrapolated measurement recovery.

---

## 6. CLI Reference & Usage Guide

### Build Instructions
```bash
# Configure and build all phases
cmake -B build
cmake --build build
```

### Running Individual Phases

#### Phase 1: Parsing & AST Dump
```bash
./build/qadvisor_phase1 inputs/para.qasm --dump-ast
```

#### Phase 2: Dependency Analysis & Parallel Scheduling
```bash
./build/qadvisor_phase2 inputs/para.qasm
```
*Outputs dependency graph, cycle groupings, gate-to-cycle mapping, and theoretical speedup.*

#### Phase 3: QIR Code Generation
```bash
./build/qadvisor_phase3 inputs/para.qasm -o output.ll -O2 --verify
```
*Options:*
* `-o, --output <file>`: Output path for LLVM IR bitcode.
* `-O, --optimize <level>`: LLVM optimization level (0, 1, 2, 3).
* `--dump-ast`: Print AST.
* `--dump-deps`: Print dependency graph.
* `--dump-schedule`: Print parallel schedule.
* `--verify`: Run LLVM module verification.

### End-to-End Pipeline Execution
```bash
./compile_and_run.sh inputs/para.qasm
```
*Executes Phase 1 $\to$ Phase 2 $\to$ Phase 3, generates `output.ll`, and triggers `qir-executor/main.py` to run the statevector simulation and report bitstring probabilities.*


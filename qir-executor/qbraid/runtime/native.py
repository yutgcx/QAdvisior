import subprocess
import math
import re

class QirRunner:
    def __init__(self, seed=None, exec_path=None):
        self._qir_runner = exec_path
        self._version = "true-statevector-mock"

    def execute(self, file_path, shots=1024, entrypoint="main"):
        print(f"Compiling LLVM IR '{file_path}' with ad-hoc runtime framework...")
        output_bin = "./qir_simulator_run_temp"
        
        compile_cmd = ["clang++", file_path, "../test/qir_runtime.cpp", "-o", output_bin]
        subprocess.run(compile_cmd, check=True, stderr=subprocess.PIPE)
        
        print("Executing compiled runtime natively:")
        print("-----------------------------------------")
        res = subprocess.run([output_bin], capture_output=True, text=True)
        out = res.stdout
        print(out, end='')
        print("-----------------------------------------")
        
        # Calculate Number of Qubits Dynamically
        num_qubits = 0
        lines = out.strip().split('\n')
        for line in lines:
            if "on Qubit ->" in line:
                q_part = line.split("on Qubit ->")[1].strip()
                q = int(q_part.split()[0])
                num_qubits = max(num_qubits, q + 1)
            elif "Target Qubit ->" in line:
                q_part = line.split("Target Qubit ->")[1].strip()
                q = int(q_part.split()[0])
                num_qubits = max(num_qubits, q + 1)
                
        if num_qubits == 0:
            return {"0": shots}
            
        # Initialize State Vector for N qubits
        state = [0.0j] * (2**num_qubits)
        state[0] = 1.0 + 0.0j
        
        # State Vector Gate Operations
        def apply_h(target):
            sqrt2_inv = 1.0 / math.sqrt(2)
            for i in range(len(state)):
                if (i & (1 << target)) == 0:
                    i1 = i | (1 << target)
                    a = state[i]
                    b = state[i1]
                    state[i] = (a + b) * sqrt2_inv
                    state[i1] = (a - b) * sqrt2_inv
                    
        def apply_x(target):
            for i in range(len(state)):
                if (i & (1 << target)) == 0:
                    i1 = i | (1 << target)
                    a = state[i]
                    b = state[i1]
                    state[i] = b
                    state[i1] = a
                    
        def apply_z(target):
            for i in range(len(state)):
                if (i & (1 << target)) != 0:
                    state[i] = -state[i]
                    
        def apply_cx(control, target):
            for i in range(len(state)):
                if (i & (1 << control)) != 0:
                    if (i & (1 << target)) == 0:
                        i1 = i | (1 << target)
                        a = state[i]
                        b = state[i1]
                        state[i] = b
                        state[i1] = a

        # Run Simulator Step By Step
        for line in lines:
            if "Executing Hadamard" in line:
                q_part = line.split("on Qubit ->")[1].strip()
                apply_h(int(q_part.split()[0]))
            elif "Executing Pauli-X" in line:
                q_part = line.split("on Qubit ->")[1].strip()
                apply_x(int(q_part.split()[0]))
            elif "Executing Pauli-Z" in line:
                q_part = line.split("on Qubit ->")[1].strip()
                apply_z(int(q_part.split()[0]))
            elif "Executing CNOT" in line:
                c_part = line.split("Control Qubit ->")[1]
                c = int(c_part.split("/")[0].strip())
                t_part = c_part.split("Target Qubit ->")[1].strip()
                apply_cx(c, int(t_part.split()[0]))
                
        # Generate distribution based on exact state vector outcome
        import random
        probs = [abs(c)**2 for c in state]
        result_counts = {}
        
        for idx, p in enumerate(probs):
            bs = bin(idx)[2:].zfill(num_qubits)
            count = int(round(p * shots))
            result_counts[bs] = count
                    
        return result_counts

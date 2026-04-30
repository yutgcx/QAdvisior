#include <iostream>
#include <cstdint>

extern "C" {
    // Declarations for Qubit mapping
    void __quantum__qis__h__body(void* qubit) {
        std::cout << "[QIR Runner] Executing Hadamard (H) gate on Qubit -> " << (uint64_t)qubit << std::endl;
    }
    
    void __quantum__qis__x__body(void* qubit) {
        std::cout << "[QIR Runner] Executing Pauli-X gate on Qubit -> " << (uint64_t)qubit << std::endl;
    }
    
    void __quantum__qis__z__body(void* qubit) {
        std::cout << "[QIR Runner] Executing Pauli-Z gate on Qubit -> " << (uint64_t)qubit << std::endl;
    }
    
    void __quantum__qis__cnot__body(void* control, void* target) {
        std::cout << "[QIR Runner] Executing CNOT (CX) gate with Control Qubit -> " << (uint64_t)control 
                  << " / Target Qubit -> " << (uint64_t)target << std::endl;
    }
    
    void __quantum__qis__mz__body(void* qubit, void* result) {
        std::cout << "[QIR Runner] Executing Measure (MZ) gate on Qubit -> " << (uint64_t)qubit 
                  << " storing to Classical Result -> " << (uint64_t)result << std::endl;
    }
}

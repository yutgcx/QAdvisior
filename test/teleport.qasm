OPENQASM 2.0;
include "qelib1.inc";

qreg q[3];
creg c[3];

// Preparation: put q[0] in a superposition state
h q[0];

// Entangle q[1] and q[2]
h q[1];
cx q[1], q[2];

// Teleportation protocol
cx q[0], q[1];
h q[0];

// Deferred measurement (pure quantum implementation)
cx q[1], q[2];
cz q[0], q[2];

// Measurement
measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];

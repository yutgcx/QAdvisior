

qreg q[4];
creg c[4];

x q[2];
h q[0];
cx q[1], q[2];
cx q[0], q[1];
h q[2];

measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];

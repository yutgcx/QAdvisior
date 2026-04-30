; ModuleID = 'quantum_circuit'
source_filename = "quantum_circuit"

declare void @__quantum__qis__h__body(ptr) local_unnamed_addr

declare void @__quantum__qis__cnot__body(ptr, ptr) local_unnamed_addr

declare void @__quantum__qis__mz__body(ptr, ptr) local_unnamed_addr

define void @main() local_unnamed_addr {
entry:
  tail call void @__quantum__qis__h__body(ptr null)
  tail call void @__quantum__qis__cnot__body(ptr null, ptr nonnull inttoptr (i64 1 to ptr))
  tail call void @__quantum__qis__mz__body(ptr null, ptr null)
  tail call void @__quantum__qis__mz__body(ptr nonnull inttoptr (i64 1 to ptr), ptr nonnull inttoptr (i64 1 to ptr))
  ret void
}

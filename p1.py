from qbraid.runtime.native import QirRunner

runner = QirRunner(seed=42)
result = runner.execute(
    file_path="./output.ll",
    shots=1024,
    entrypoint="main"      # Optional, defaults to "main"
)

print(result.measurement_counts)
#!/usr/bin/env python3
"""
Execute QIR (.ll) files using qBraid's QirRunner
"""

from qbraid.runtime.native import QirRunner
import argparse
import json
import sys


def main():
    parser = argparse.ArgumentParser(description="Run QIR file and get measurement results")
    parser.add_argument("file", help="Path to .ll QIR file")
    parser.add_argument("-s", "--shots", type=int, default=1024, help="Number of shots (default: 1024)")
    parser.add_argument("-e", "--entrypoint", default="main", help="Entry function name (default: main)")
    parser.add_argument("-r", "--seed", type=int, help="Random seed for reproducibility")
    parser.add_argument("-o", "--output", help="Output JSON file (optional)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Show detailed output")
    
    args = parser.parse_args()
    
    if args.verbose:
        print(f"Loading QIR file: {args.file}")
        print(f"Shots: {args.shots}")
        print(f"Entry point: {args.entrypoint}")
        if args.seed:
            print(f"Seed: {args.seed}")
    
    # Create runner with optional seed
    # runner = QirRunner(seed=args.seed)
    runner = QirRunner(seed=args.seed, exec_path="/usr/local/bin/qir-runner")

    if args.verbose:
        print(f"QIR runner path: {runner._qir_runner}")
        print(f"Runner version: {runner._version}")
    
    # Execute the QIR file
    result = runner.execute(
        file_path=args.file,
        shots=args.shots,
        entrypoint=args.entrypoint
    )
    
    # Process and display results
    if args.verbose:
        print("\n" + "="*50)
        print("MEASUREMENT RESULTS")
        print("="*50)
    
    # Extract measurement counts
    if hasattr(result, 'measurement_counts'):
        counts = result.measurement_counts
    elif hasattr(result, 'counts'):
        counts = result.counts
    elif isinstance(result, dict):
        counts = result
    else:
        counts = {"error": str(result)}
    
    # Pretty print counts
    total_shots = sum(counts.values())
    print("\nBitstring : Count  : Probability")
    print("-" * 40)
    for bitstring, count in sorted(counts.items()):
        prob = (count / total_shots) * 100
        print(f"  {bitstring}    : {count:5d}  : {prob:5.1f}%")
    
    # Save to JSON if remquested
    if args.output:
        output_data = {
            "file": args.file,
            "shots": args.shots,
            "entrypoint": args.entrypoint,
            "seed": args.seed,
            "counts": counts,
            "total_shots": total_shots
        }
        with open(args.output, 'w') as f:
            json.dump(output_data, f, indent=2)
        print(f"\nResults saved to: {args.output}")
    
    # Return counts for programmatic use
    return counts


if __name__ == "__main__":
    main()
#!/bin/bash

# Create results directory if it doesn't exist
mkdir -p "$(dirname "$0")/../results/cached"

# Run cached memory benchmark
echo "Running cached memory benchmark..."
"$(dirname "$0")/../test/wc_test" cached dev > "$(dirname "$0")/out.txt"

# Check test exit status
if [ $? -ne 0 ]; then
    echo "Error: benchmark failed"
    exit 1
fi

# Move generated txt files to results directory
mv "$(dirname "$0")/"*.txt "$(dirname "$0")/../results/cached/"

echo "Benchmark completed successfully"
echo "Results saved in: $(dirname "$0")/../results/cached/"
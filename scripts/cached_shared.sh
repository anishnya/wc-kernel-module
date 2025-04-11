#!/bin/bash

# Create results directory if it doesn't exist
mkdir -p "$(dirname "$0")/../results/shm"

# Define buffer sizes (in bytes)
SIZES=(
    64      # 64B
    128     # 128B
    1024    # 1KB
    4096    # 4KB
    32768   # 32KB (L1)
    65536   # 64KB
    262144  # 256KB
)

echo "Running shared memory benchmark..."

# Loop through each buffer size
for size in "${SIZES[@]}"; do
    echo "Testing buffer size: $size bytes"
    "$(dirname "$0")/../test/shm_benchmark" "$size" > "$(dirname "$0")/out_${size}.txt"
    
    # Check test exit status
    if [ $? -ne 0 ]; then
        echo "Error: benchmark failed for size $size"
        exit 1
    fi
done

# Move all generated txt files to results directory
mv "$(dirname "$0")"/*.txt "$(dirname "$0")"/../results/shm/

echo "Benchmark completed successfully"
echo "Results saved in: $(dirname "$0")/../results/shm/"
#!/bin/bash


# Define buffer sizes (in bytes)
SIZES=(
    262144      # 256 KB
    524288      # 512 KB
    786432      # 768 KB
    1048576     # 1 MB
    1310720     # 1.25 MB
    1572864     # 1.5 MB
    1835008     # 1.75 MB
    2097152     # 2 MB
    2359296     # 2.25 MB
    2621440     # 2.5 MB
    2883584     # 2.75 MB
    3145728     # 3 MB
)

echo "Running shared memory benchmark..."

# Loop through each thread count and buffer size
for threads in {1..1}; do
    echo "Testing with $threads thread(s)"
    # Create results directory if it doesn't exist
    mkdir -p "$(dirname "$0")/../results_v2/shm"
    
    for size in "${SIZES[@]}"; do
        echo "Testing buffer size: $size bytes"
        "$(dirname "$0")/../test/shm_benchmark" "$size" "$threads" > "$(dirname "$0")/out_${size}.txt"
        
        # Check test exit status
        if [ $? -ne 0 ]; then
            echo "Error: benchmark failed for size $size with $threads thread(s)"
            exit 1
        fi
    done

    # Move all generated txt files to results directory
    mv "$(dirname "$0")"/*.txt "$(dirname "$0")"/../results_v2/shm/

    echo "Benchmark completed successfully"
    echo "Results saved in: $(dirname "$0")/../results_v2/shm/"
done


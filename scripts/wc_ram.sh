#!/bin/bash
SCRIPT_DIR=$(realpath "$(dirname "$0")")

# Define 2D array of flush configurations using associative array
declare -A FLUSH_CONFIGS=(
    # ["0,0"]="disabled-disabled"
    # ["0,1"]="disabled-enabled"
    # ["1,0"]="enabled-disabled"
    ["1,1"]="enabled-enabled"
)


# Run cached memory benchmark for each configuration
echo "Running cached memory benchmark..."
for config in "${!FLUSH_CONFIGS[@]}"; do
    temporal=${config%,*}
    non_temporal=${config#*,}
    desc=${FLUSH_CONFIGS[$config]}

    # Create results directory if it doesn't exist
    mkdir -p "${SCRIPT_DIR}/../results_v2/wc_${desc}"
    
    echo "Running with temporal_flush=${temporal}, non_temporal_flush=${non_temporal} (${desc})"
    "${SCRIPT_DIR}/../test/wc_test" uncached "${SCRIPT_DIR}/dev" "${temporal}" "${non_temporal}" > "${SCRIPT_DIR}/out_${desc}.txt"
    
    # Check test exit status
    if [ $? -ne 0 ]; then
        echo "Error: benchmark failed with config ${desc}"
        exit 1
    fi

    # Move generated txt files to results directory
    mv "${SCRIPT_DIR}/"*.txt "${SCRIPT_DIR}/../results_v2/wc_${desc}/"

    echo "Benchmark completed successfully"
    echo "Results saved in: ${SCRIPT_DIR}/../results_v2/wc_${desc}/"
done
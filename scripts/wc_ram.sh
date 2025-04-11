#!/bin/bash

#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")

# Create results directory if it doesn't exist
mkdir -p "${SCRIPT_DIR}/../results/cached"

# Run cached memory benchmark using absolute path
echo "Running cached memory benchmark..."
"${SCRIPT_DIR}/../test/wc_test" uncached "${SCRIPT_DIR}/dev" > "${SCRIPT_DIR}/out.txt"

# Check test exit status
if [ $? -ne 0 ]; then
    echo "Error: benchmark failed"
    exit 1
fi

# Move generated txt files to results directory
mv "${SCRIPT_DIR}/"*.txt "${SCRIPT_DIR}/../results/wc/"

echo "Benchmark completed successfully"
echo "Results saved in: ${SCRIPT_DIR}/../results/wc/"
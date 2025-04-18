import os
import re
import csv
from collections import defaultdict

def parse_latency_file(filepath):
    """Parse a latency result file and extract type, size, and percentile values."""
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    content = ' '.join(lines)
    
    # Extract percentile values and latency using regex
    percentile_pattern = r'p1 = (\d+), p10 = (\d+), p25 = (\d+), p50 = (\d+), p75 = (\d+), p90 = (\d+), p99 = (\d+)'
    latency_pattern = r'Latency = (\d+\.\d+)'
    
    percentile_match = re.search(percentile_pattern, content)
    latency_match = re.search(latency_pattern, content)
    
    if not percentile_match:
        print(f"Percentile match not found in {filepath}")
        return None
    if not latency_match:
        print(f"Latency match not found in {filepath}")
        return None
    
    # Extract values from matches
    p1, p10, p25, p50, p75, p90, p99 = map(int, percentile_match.groups())
    latency = float(latency_match.group(1))
    
    # Get relative path components
    rel_path = os.path.relpath(filepath, os.path.dirname(os.path.abspath(__file__)))
    path_parts = rel_path.split(os.sep)
    
    # Extract operation type from filename
    filename = path_parts[-1]
    op_type_match = re.match(r'([A-Za-z0-9]+)[_]{1,2}([A-Za-z]+)[_]{1,2}(\d+)', filename)
    if not op_type_match:
        print(f"Operation type match not found in {filename}")
        return None
    
    # Combine prefix and operation without underscore
    op_type = op_type_match.group(1) + " " + op_type_match.group(2)
    
    # Extract size (first number in filename)
    size_match = re.search(r'[_]{1,2}(\d+)(?:[_]{1,2}\d+)?(?:__core\d+)?\.txt', filename)
    if not size_match:
        return None
    
    size = int(size_match.group(1))
    
    return {
        'p1': p1,
        'p10': p10,
        'p25': p25,
        'p50': p50,
        'p75': p75,
        'p90': p90,
        'p99': p99,
        'avg_latency': latency,
        'op_type': op_type,
        'size': size,
        'filename': filename
    }

def analyze_results():
    """Analyze all result files and write results to CSV."""
    base_dir = os.path.dirname(os.path.abspath(__file__))
    results_dir = os.path.join(base_dir, 'results_v2')
    output_file = os.path.join(base_dir, 'benchmark_results_v2.csv')
    
    # Dictionary to store results by memory type
    results = defaultdict(list)
    
    # Walk through results directory
    for subdir, _, files in os.walk(results_dir):
        mem_type = os.path.basename(subdir)  # 'wc' or 'wb' or 'shm'
        print(f"Processing memory type: {mem_type}")
        
        for file in files:
            if file.endswith('.txt') and (file.startswith('NT_') or file.startswith('Regular_')) or file.startswith('AVX512_'):
                filepath = os.path.join(subdir, file)
                try:
                    result = parse_latency_file(filepath)
                    if result is None:
                        continue
                    
                    result['mem_type'] = mem_type  # Add memory type to result
                    results[mem_type].append(result)
                except Exception as e:
                    print(f"Error parsing {filepath}: {e}")

    # Write all results to CSV
    with open(output_file, 'w', newline='') as csvfile:
        fieldnames = ['mem_type', 'op_type', 'size', 'p1', 'p10', 'p25', 'p50', 'p75', 'p90', 'p99', 'avg_latency']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        
        writer.writeheader()
        for mem_type, measurements in results.items():
            for m in measurements:
                writer.writerow({
                    'mem_type': m['mem_type'],
                    'op_type': m['op_type'],
                    'size': m['size'],
                    'p1': m['p1'],
                    'p10': m['p10'],
                    'p25': m['p25'],
                    'p50': m['p50'],
                    'p75': m['p75'],
                    'p90': m['p90'],
                    'p99': m['p99'],
                    'avg_latency': m['avg_latency']
                })
    
    print(f"\nResults written to: {output_file}")

if __name__ == "__main__":
    analyze_results()
import os
import re
import csv
from collections import defaultdict

def parse_latency_file(filepath):
    """Parse a latency result file and extract type, size, and latency value."""
    with open(filepath, 'r') as f:
        first_line = f.readline()
        
    # Extract base latency from first line
    latency_match = re.search(r'Latency = ([\d.]+)', first_line)
    if not latency_match:
        raise ValueError(f"Could not find latency value in {filepath}")
    latency = float(latency_match.group(1))
    
    filename = os.path.basename(filepath)
    # Extract operation type (e.g., Regular_read, NT_write)
    op_type_match = re.match(r'((?:Regular|NT)_[a-z]+)__', filename)
    if not op_type_match:
        raise ValueError(f"Could not parse operation type from filename: {filename}")
    op_type = op_type_match.group(1)

    # Extract size
    size_match = re.search(r'__(\d+)\.txt', filename)
    if not size_match:
        raise ValueError(f"Could not parse size from filename: {filename}")
    size = int(size_match.group(1))
    
    return {
        'latency': latency,
        'op_type': op_type,
        'size': size,
        'filename': filename
    }

def analyze_results():
    """Analyze all result files and write results to CSV."""
    base_dir = os.path.dirname(os.path.abspath(__file__))
    results_dir = os.path.join(base_dir, 'results')
    output_file = os.path.join(base_dir, 'benchmark_results.csv')
    
    # Dictionary to store results by memory type
    results = defaultdict(list)
    
    # Walk through results directory
    for subdir, _, files in os.walk(results_dir):
        mem_type = os.path.basename(subdir)  # 'wc' or 'wb' or 'shm'
        
        for file in files:
            if file.endswith('.txt') and (file.startswith('NT_') or file.startswith('Regular_')):
                filepath = os.path.join(subdir, file)
                try:
                    result = parse_latency_file(filepath)
                    result['mem_type'] = mem_type  # Add memory type to result
                    results[mem_type].append(result)
                except Exception as e:
                    print(f"Error parsing {filepath}: {e}")

    # Write all results to CSV
    with open(output_file, 'w', newline='') as csvfile:
        fieldnames = ['mem_type', 'op_type', 'size', 'latency']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        
        writer.writeheader()
        for mem_type, measurements in results.items():
            for m in measurements:
                writer.writerow({
                    'mem_type': m['mem_type'],
                    'op_type': m['op_type'],
                    'size': m['size'],
                    'latency': f"{m['latency']:.2f}"
                })
    
    print(f"\nResults written to: {output_file}")

if __name__ == "__main__":
    analyze_results()
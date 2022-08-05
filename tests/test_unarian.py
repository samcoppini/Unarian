#!/usr/bin/env python3

import argparse
import re
import subprocess
import sys
from typing import Dict, TextIO

def get_inputs(file: TextIO) -> Dict[str, str]:
    inputs: Dict[str, str] = {}

    for line in file.readlines():
        match = re.match(r'.*#\s*input:\s*(\d+)\s*->\s*(\d+|-)', line)

        if match is not None:
            inputs[match.group(1)] = match.group(2)

    return inputs

def run_test(exe_path: str, test_path: str) -> int:
    with open(test_path, 'r') as test_file:
        inputs = get_inputs(test_file)

    had_failure = False

    for input, output in inputs.items():
        input_bytes = bytes(input, 'utf-8')

        proc = subprocess.run([exe_path, test_path, '-ig'], input=bytes(input, 'utf-8'), capture_output=True)
        actual_output = proc.stdout

        print(f'Expected {output} for input {input}. Received {str(actual_output, "utf-8")}')
        if actual_output[:-1] != bytes(output, 'utf-8'):
            had_failure = True

    return 1 if had_failure else 0

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', type=str)
    parser.add_argument('--test', type=str)
    args = parser.parse_args()

    return run_test(args.exe, args.test)

if __name__ == '__main__':
    sys.exit(main())
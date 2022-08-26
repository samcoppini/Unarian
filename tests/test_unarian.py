#!/usr/bin/env python3

#
#  Copyright 2022 Sam Coppini
#
#  Distributed under the Boost Software License, Version 1.0.
#  See accompanying file LICENSE.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt
#

import argparse
from collections import namedtuple
import re
import subprocess
import sys
from typing import Dict, TextIO

InputKey = namedtuple('InputKey', ['expr', 'input'])

def get_inputs(file: TextIO) -> Dict[InputKey, str]:
    inputs: Dict[str, str] = {}

    for line in file.readlines():
        program_match = re.match(r'^[^#\n ]+', line)
        if program_match is not None:
            program_name = program_match.group(0)

        input_match = re.match(r'.*#\s*input:\s*(\d+)\s*->\s*(\d+|-)', line)

        if input_match is not None:
            inputs[(program_name, input_match.group(1))] = input_match.group(2)

    return inputs

def run_test(exe_path: str, test_path: str) -> int:
    with open(test_path, 'r') as test_file:
        inputs = get_inputs(test_file)

    had_failure = False

    for input, output in inputs.items():
        expr, input_num = input

        proc = subprocess.run([exe_path, test_path, '-ige', expr], input=bytes(input_num, 'utf-8'), capture_output=True)
        actual_output = str(proc.stdout, 'utf-8').strip()

        if actual_output != output:
            print(f'Expected {output} for input {input_num} for {expr}. Received {actual_output}')
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

#!/usr/bin/env python

import sys
from collections import defaultdict

sep = "-----------------------------------"


def print_lines(lines):
    for l in lines:
        print(l)


def process_file(f):
    instructions = defaultdict(list)
    instruction = []
    for line in f:
        line = line.strip()
        if line == sep:
            if len(instruction) > 1 and instruction[0].startswith("0"):
                key = instruction[0]
                instructions[key].append(instruction)
            instruction = []
        else:
            instruction.append(line)

    for k, v in sorted(instructions.items()):
        print(sep)
        annotations = []
        annotations.append(f"> Count {len(v)}")
        if "Bcc " in k and "cond:True" not in k and len(v) > 1:
            ever_taken = False
            ever_not_taken = False
            for sample in v:
                if "Branch taken" in sample:
                    ever_taken = True
                else:
                    ever_not_taken = True
            if ever_taken and not ever_not_taken:
                annotations.append("> Always taken")
            elif not ever_taken and ever_not_taken:
                annotations.append("> Never taken")

        lines = v[0]
        if len(lines) < 40:
            print_lines(lines)
        else:
            print_lines(lines[:20])
            print("...")
            print_lines(lines[-20:])
        print_lines(annotations)


def main():
    if len(sys.argv) <= 1:
        process_file(sys.stdin)
    else:
        for file in sys.argv[1:]:
            with open(file) as f:
                process_file(f)


if __name__ == "__main__":
    main()

#!/usr/bin/env python

import sys

from collections import defaultdict


def main():
    lines = defaultdict(lambda: 0)
    for line in sys.stdin:
        lines[line.strip()] += 1

    lines = sorted(lines.items(), key=lambda x: (x[1], x[0]))
    for (line, c) in lines:
        print(f'{c: 9d} {line}')


if __name__ == "__main__":
    main()

#!/usr/bin/python
import sys
num = -1
refFile = "benchmark-ref.txt"
if len(sys.argv) > 1:
    refFile = sys.argv[1]

epsilon = .15


def prettyPrint(name, time, ref):
    delta = ref-time
    if delta < -epsilon:
        color = "\x1b[31m"  # red
    elif delta > epsilon:
        color = "\x1b[32m"  # green
    else:
        color = ""

    print("{:15s} {} {:6.03f} -> {:6.03f} {:6.2f}\x1b[0m".format(
        name, color, ref, time, delta))


with sys.stdin as current:
    with open(refFile) as ref:
        i = 0
        while i != num:
            i += 1
            line = current.readline()
            if not line:
                break
            name, dt = line.split(": ")
            refLine = ref.readline()
            refName, refDt = refLine.split(": ")
            if name != refName:
                continue
            prettyPrint(name, float(dt), float(refDt))

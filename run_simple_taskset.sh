#!/bin/bash

# ./clustering_launcher simple_taskset 

# strace -o trace_version1.txt -e trace=all ./clustering_launcher simple_taskset

strace -o trace_output.txt ./clustering_launcher simple_taskset

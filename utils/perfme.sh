#!/bin/sh

perf record -g "$@"
perf annotate -d $1
perf report --demangle -g --source

#!/usr/bin/env bash

# A script that builds your compiler.
cd "$(dirname $0)"/../../src
make -B all

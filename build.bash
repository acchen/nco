#!/usr/bin/env bash

# Bash commands to compile and install the code locally.
#
# By: Albert Chen

./configure --prefix=$HOME/local
make
make install

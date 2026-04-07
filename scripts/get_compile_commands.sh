#!/bin/bash

rm compile_commands.json

python3 -m venv .compiledbenv
source .compiledbenv/bin/activate
pip install compiledb
make clean
make VERBOSE=1 -j 8 -B > ./build_log.txt
compiledb < build_log.txt
deactivate

rm build_log.txt
rm -rf .compiledbenv
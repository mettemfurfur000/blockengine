#!/bin/bash
# Usage: ./check_c_funcs.sh <directory>
# Example: ./check_c_funcs.sh ./src

LINE_LIMIT=50
INDENT_LIMIT=3

find "$1" -name "*.c" -print0 | xargs -0 awk -v ll="$LINE_LIMIT" -v il="$INDENT_LIMIT" '
# Detect function start: assumes non-indented, starts with type, ends with ) {
/^[a-zA-Z_]/ && /[a-zA-Z_0-9][*]*[ )]+[a-zA-Z0-9_]+[ ]*\([^\)]*\)[ ]*$/ {
    if (func_name != "") {
        check_func()
    }
    func_name = $0
    func_start = NR
    indent_max = 0
    in_func = 1
    next
}
# Count indentation and lines within function
in_func && /^\t/ {
    match($0, /^\t+/)
    indent_level = RLENGTH
    if (indent_level > indent_max) indent_max = indent_level
}
# End function at closing brace
/^}/ {
    if (in_func) {
        check_func()
        in_func = 0
        func_name = ""
    }
}
function check_func() {
    len = NR - func_start
    if (len > ll || indent_max > il) {
        printf "File: %s | Func: %s | Lines: %d | Max Indent: %d\n", FILENAME, func_name, len, indent_max
    }
}
'
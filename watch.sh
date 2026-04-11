#!/bin/bash

WATCH_DIRS="./src ./mains ./include ./libs"
COMPILE_COMMAND="make"
SLEEP_INTERVAL=2
BUILD_LOG="build/compile.log"
TIMESTAMP_FILE="build/timestamp.txt"

get_state() {
    find $WATCH_DIRS -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" \) -print0 | xargs -0 md5sum
}

LAST_STATE=$(get_state)

echo "Watching directories: $WATCH_DIRS"
echo "Compile command: $COMPILE_COMMAND"

while true; do
    sleep "$SLEEP_INTERVAL"
    CURRENT_STATE=$(get_state)

    if [ "$CURRENT_STATE" != "$LAST_STATE" ]; then
        echo "File changes detected! Recompiling..."
        date +"%Y-%m-%d %H:%M:%S" > "$TIMESTAMP_FILE"
        "$COMPILE_COMMAND" 2>&1 | tee "$BUILD_LOG"
        LAST_STATE=$CURRENT_STATE
    fi
done

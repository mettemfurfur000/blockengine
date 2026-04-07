#!/bin/bash

binary_path=$1
dll_folder=$2
depth=$3
out_path=$(dirname $binary_path)

# Black‑list of DLLs that are guaranteed to exist on every Windows installation
SYSTEM_DLLS=(
    kernel32 advapi32 user32 gdi32 ws2_32
    comdlg32 shell32 ole32 oleaut32
    ntdll msvcrt bcrypt crypt32
    winmm imm32 setupapi version 
    SHLWAPI
)

# Turn the array into a regex pattern:  ^(kernel32|advapi32|user32)\.dll$
SYS_REGEX="^($(printf "%s|" "${SYSTEM_DLLS[@]}" | sed 's/|$//')).dll$"

dlls=$(llvm-readobj.exe --coff-imports "$binary_path" \
        | grep -i "Name:" \
        | awk '{print $2}' \
        | sed 's/\\//g' \
        | grep -viE "$SYS_REGEX")

# echo "$dlls"

printf "%s\n" $dlls | \
    while IFS= read -r dll; do
        [[ -e "$dll_folder/$dll" ]] && printf '%s\n' "$dll_folder/$dll"
    done | \
    xargs -P$(nproc) -I{} cp -p {} "$out_path/"

if (( depth > 0 )); then
    for dll in $dlls; do
        [[ -e "$dll_folder/$dll" ]] && \
            "$0" "$out_path/$dll" "$dll_folder" $((depth-1))
    done
fi

echo "grabbed $dlls"
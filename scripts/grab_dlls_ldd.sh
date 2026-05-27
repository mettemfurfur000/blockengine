#!/usr/bin/env bash
set -euo pipefail

# Fast DLL grabber using ldd
# Usage: grab_dlls_ldd.sh <binary_path> <dll_folder> [depth]

if [ $# -lt 2 ]; then
  echo "Usage: $0 <binary_path> <dll_folder> [depth]" >&2
  exit 2
fi

binary_path=$1
dll_folder=$2
depth=${3:-0}
out_path=$(dirname "$binary_path")

if ! command -v ldd >/dev/null 2>&1; then
  echo "ldd not found in PATH" >&2
  exit 1
fi

# Common system DLLs to ignore (lowercase names)
SYSTEM_DLLS=(kernel32 advapi32 user32 gdi32 ws2_32 comdlg32 shell32 ole32 oleaut32 ntdll msvcrt bcrypt crypt32 winmm imm32 setupapi version shlwapi)
SYS_REGEX="^($(printf "%s|" "${SYSTEM_DLLS[@]}" | sed 's/|$//')).*(\\.dll|\\.so)?$"

ldd "$binary_path" 2>/dev/null | while IFS= read -r line; do
  [[ -z "$line" ]] && continue
  if printf '%s' "$line" | grep -q '=>'; then
    candidate=$(printf '%s' "$line" | sed -n 's/.*=> *\([^ ]*\).*/\1/p')
  else
    candidate=$(printf '%s' "$line" | awk '{print $1}')
  fi
  candidate=${candidate%%(*}
  candidate=$(printf '%s' "$candidate" | xargs)
  name=$(basename "$candidate")
  name_lower=$(printf '%s' "$name" | tr '[:upper:]' '[:lower:]')

  # skip those that have system paths in it

  if printf '%s\n' "$candidate" | grep -qiE '/(lib|usr|windows|system32|syswow64)/'; then
    continue
  fi

  # skip system DLLs
  if printf '%s\n' "$name_lower" | grep -qiE "$SYS_REGEX"; then
    continue
  fi

  # prefer absolute path reported by ldd if present, otherwise look in supplied folder
  if [[ -n "$candidate" && -f "$candidate" ]]; then
    src="$candidate"
  elif [[ -f "$dll_folder/$name" ]]; then
    src="$dll_folder/$name"
  else
    # not found in provided folder, skip
    continue
  fi

  printf 'Copying %s -> %s\n' "$src" "$out_path/"
  cp -p "$src" "$out_path/" || true
done

if (( depth > 0 )); then
  for f in "$out_path"/*.{dll,so}; do
    [[ -f "$f" ]] || continue
    "$0" "$f" "$dll_folder" $((depth-1))
  done
fi

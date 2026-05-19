#!/bin/sh

success_count=0
total_attempts=0

if [ -f "$1" ]; then
  echo "Running for ${1}"
  clang -xc -I -w -resource-dir="$CLANG_RESOURCES" -fsyntax-only verifier.c "$1" > /dev/null 2>&1
  exit 0
elif [ -d "$1" ]; then
  echo "Running for Dir: ${1}"
  temp_results=$(mktemp)

  find "$1" -name "*.c" -type f -exec sh -c '
    file="$0"
    clang -xc -I -w -resource-dir="$CLANG_RESOURCES" -fsyntax-only verifier.c "$file" > /dev/null 2>&1
    if [ $? -eq 0 ]; then
      echo "SUCCESS:$file"
    else
      echo "ERROR:$file"
    fi
  ' {} \; > "$temp_results"

  while IFS= read -r line; do
    total_attempts=$((total_attempts + 1))
    if echo "$line" | grep -q "^SUCCESS:"; then
      # echo " $(echo "$line" | sed "{s/$success//}")"
      echo "$line"
      success_count=$((success_count + 1))
    elif echo "$line" | grep -q "^ERROR:"; then
      # echo " $(echo "$line" | sed "{s/$errors//}")"
      echo "$line"
    fi
  done < "$temp_results"
  rm -f "$temp_results"
fi
echo ""
echo "--- Summary ---"
echo "Total Files Checked: $total_attempts"
echo "Total Successes: $success_count"

exit "$success_count"

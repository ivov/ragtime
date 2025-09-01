#!/bin/bash

echo "Available targets:"

# Read the Makefile and extract comments followed by targets
while IFS= read -r line; do
  if [[ $line =~ ^##[[:space:]](.*)$ ]]; then
    desc="${BASH_REMATCH[1]}"
  elif [[ $line =~ ^([a-zA-Z_-]+): ]] && [[ -n $desc ]]; then
    target="${BASH_REMATCH[1]}"
    printf "  %-10s %s\n" "$target" "$desc"
    desc=""
  elif [[ $line =~ ^[[:space:]]*$ ]]; then
    desc=""
  fi
done <"$1"

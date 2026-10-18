#!/bin/bash

# Default to all images in images/ if no arguments provided
if [ "$#" -eq 0 ]; then
    set -- images/*
elif [ "$#" -eq 1 ] && [ -d "$1" ]; then
    # If the argument is a directory (e.g., "./run_demo.sh images"), use its contents
    set -- "$1"/*
fi

# Ensure output directory exists
mkdir -p output

echo "--- Launching Clients in Parallel ---"

# Loop through arguments and launch clients
count=1
for img in "$@"; do
    # Skip directories and non-image files
    if [ -d "$img" ] || [[ ! "$img" =~ \.(png|jpg|jpeg|bmp)$ ]]; then
        continue
    fi

    echo "[Launcher] Spawning client for: $img (ID: $count)"
    ./client "$img" "$count" &  # Run in background
    count=$((count + 1))
done

# Wait for all background processes to finish
wait
echo "--- All clients finished ---"

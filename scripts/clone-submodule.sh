#!/bin/bash

REPO_PATH=$1
SRCDIR=$2

# Convert REPO_PATH to a relative path with respect to SRCDIR
RELATIVE_PATH=$(realpath --relative-to="$SRCDIR" "$REPO_PATH")

echo "Attempting shallow submodule update..."
if git submodule update --init --depth 1 --progress $RELATIVE_PATH; then
    echo "Shallow submodule update succeeded!"
else
    echo "Shallow submodule update failed. Cleaning up and retrying with full submodule update..."

    # Deinitialize the submodule
    git submodule deinit -f -- $RELATIVE_PATH
    rm -rf .git/modules/$RELATIVE_PATH
    git prune

    # Perform a fresh submodule update
    git submodule update --init --progress $RELATIVE_PATH
    
    # Check if the full update succeeded
    if [ $? -eq 0 ]; then
        echo "Full submodule update succeeded after cleanup!"
    else
        echo "Full submodule update failed even after cleanup."
        exit 1
    fi
fi

#!/bin/bash

# Apply BSEC timing fix automatically
# This script suppresses the BSEC timing warnings without modifying the library permanently

set -e

BSEC_DIR="${BSEC_DIR:-/home/ubuntu/bsec-pi4-build}"
PATCH_FILE="$(dirname "$0")/bsec_timing_fix.patch"

echo "Applying BSEC timing fix..."

if [ ! -d "$BSEC_DIR" ]; then
    echo "ERROR: BSEC directory not found: $BSEC_DIR"
    echo "Please set BSEC_DIR environment variable"
    exit 1
fi

if [ ! -f "$PATCH_FILE" ]; then
    echo "ERROR: Patch file not found: $PATCH_FILE"
    exit 1
fi

# Create backup if it doesn't exist
if [ ! -f "$BSEC_DIR/src/bsec_integration.c.original" ]; then
    echo "Creating backup of original bsec_integration.c..."
    cp "$BSEC_DIR/src/bsec_integration.c" "$BSEC_DIR/src/bsec_integration.c.original"
fi

# Check if patch is already applied
if grep -q "Timing warning suppressed" "$BSEC_DIR/src/bsec_integration.c" 2>/dev/null; then
    echo "BSEC timing fix already applied"
    exit 0
fi

# Apply the patch
echo "Applying patch to suppress BSEC timing warnings..."
cd "$BSEC_DIR"
patch -p0 < "$(realpath "$PATCH_FILE")"

echo "BSEC timing fix applied successfully!"
echo "The warnings are now suppressed - the application will function normally."
echo "Note: This only suppresses warnings; actual timing precision should be maintained through system optimizations."

#!/usr/bin/env bash
set -euo pipefail

echo "=== BSEC (Bosch Sensortec Environmental Cluster) Validator ==="
echo "This script validates that BSEC is properly installed and accessible."
echo

# Check if BSEC_DIR is set
if [ -z "${BSEC_DIR:-}" ]; then
    echo "❌ ERROR: BSEC_DIR environment variable is not set."
    echo
    echo "Please set BSEC_DIR to point to your BSEC installation:"
    echo "  export BSEC_DIR=/path/to/your/BSEC"
    echo
    echo "To download BSEC:"
    echo "1. Go to Bosch Sensortec website"
    echo "2. Search for 'BSEC Software Library'"
    echo "3. Accept the license and download the package"
    echo "4. Extract it and set BSEC_DIR to the extracted directory"
    exit 1
fi

echo "✓ BSEC_DIR is set to: $BSEC_DIR"

# Check if directory exists
if [ ! -d "$BSEC_DIR" ]; then
    echo "❌ ERROR: Directory $BSEC_DIR does not exist"
    exit 1
fi
echo "✓ BSEC directory exists"

# Check for required subdirectories
missing_dirs=()

# Check for include/header directory (could be inc/, include/, or src/)
header_dir=""
for dir in "inc" "include" "src"; do
    if [ -d "$BSEC_DIR/$dir" ]; then
        header_dir="$BSEC_DIR/$dir"
        break
    fi
done

if [ -z "$header_dir" ]; then
    missing_dirs+=("inc/ or include/ or src/")
else
    echo "✓ Headers directory found: $header_dir"
fi

# Check for library directory
lib_dir=""
for dir in "lib" "binaries"; do
    if [ -d "$BSEC_DIR/$dir" ]; then
        lib_dir="$BSEC_DIR/$dir"
        break
    fi
done

if [ -z "$lib_dir" ]; then
    missing_dirs+=("lib/ or binaries/")
else
    echo "✓ Library directory found: $lib_dir"
fi

if [ ${#missing_dirs[@]} -gt 0 ]; then
    echo "❌ ERROR: Missing required directories: ${missing_dirs[*]}"
    echo
    echo "Your BSEC directory should contain:"
    echo "  - inc/ or include/ or src/ (for header files)"
    echo "  - lib/ or binaries/ (for library files)"
    exit 1
fi

# Check for required header files
echo
echo "Checking for required header files..."
required_headers=("bsec_interface.h" "bsec_datatypes.h" "bme68x.h" "bme68x_defs.h")
missing_headers=()

for header in "${required_headers[@]}"; do
    found=false
    for search_dir in "$header_dir" "$BSEC_DIR/inc" "$BSEC_DIR/include" "$BSEC_DIR/src"; do
        if [ -f "$search_dir/$header" ]; then
            echo "✓ Found $header in $search_dir"
            found=true
            break
        fi
    done
    
    if [ "$found" = false ]; then
        missing_headers+=("$header")
    fi
done

if [ ${#missing_headers[@]} -gt 0 ]; then
    echo "❌ ERROR: Missing required header files: ${missing_headers[*]}"
    exit 1
fi

# Check for library file
echo
echo "Checking for BSEC library..."
lib_found=false
lib_paths=()

# Search for library in various locations and with various names
search_dirs=("$lib_dir" "$BSEC_DIR/lib/gcc" "$BSEC_DIR/lib/Release" "$BSEC_DIR")
lib_names=("libalgobsec.a" "libbsec.a" "algobsec" "bsec")

for search_dir in "${search_dirs[@]}"; do
    if [ ! -d "$search_dir" ]; then continue; fi
    
    for lib_name in "${lib_names[@]}"; do
        if [ -f "$search_dir/$lib_name" ]; then
            echo "✓ Found BSEC library: $search_dir/$lib_name"
            lib_found=true
            lib_paths+=("$search_dir/$lib_name")
        fi
    done
done

if [ "$lib_found" = false ]; then
    echo "❌ ERROR: Could not find BSEC library file"
    echo "Searched for: ${lib_names[*]}"
    echo "In directories: ${search_dirs[*]}"
    exit 1
fi

# Check for source files
echo
echo "Checking for required source files..."
required_sources=("bme68x.c" "bsec_integration.c")
missing_sources=()

for source in "${required_sources[@]}"; do
    found=false
    for search_dir in "$header_dir" "$BSEC_DIR/inc" "$BSEC_DIR/include" "$BSEC_DIR/src"; do
        if [ -f "$search_dir/$source" ]; then
            echo "✓ Found $source in $search_dir"
            found=true
            break
        fi
    done
    
    if [ "$found" = false ]; then
        missing_sources+=("$source")
    fi
done

if [ ${#missing_sources[@]} -gt 0 ]; then
    echo "❌ ERROR: Missing required source files: ${missing_sources[*]}"
    exit 1
fi

echo
echo "🎉 SUCCESS: BSEC installation appears to be complete and valid!"
echo
echo "You can now build the project with:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make"
echo
echo "Or with explicit BSEC_DIR:"
echo "  cmake -DBSEC_DIR=$BSEC_DIR .."


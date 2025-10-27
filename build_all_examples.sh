#!/bin/bash

# ESP-IDF Components - Build All Examples Script
# This script compiles all example projects in the components repository

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR_NAME="build"
PARALLEL_JOBS=${PARALLEL_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}

# Counters
TOTAL_EXAMPLES=0
SUCCESSFUL_BUILDS=0
FAILED_BUILDS=0

# Arrays to track results
SUCCESSFUL_EXAMPLES=()
FAILED_EXAMPLES=()

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to print section headers
print_header() {
    local message=$1
    echo
    print_status $BLUE "============================================"
    print_status $BLUE "$message"
    print_status $BLUE "============================================"
}

# Function to check prerequisites
check_prerequisites() {
    print_header "Checking Prerequisites"
    
    if [ -z "$IDF_PATH" ]; then
        print_status $RED "ERROR: IDF_PATH environment variable is not set!"
        print_status $YELLOW "Please source the ESP-IDF environment:"
        print_status $YELLOW "  . \$HOME/esp/esp-idf/export.sh"
        exit 1
    fi
    
    if ! command -v idf.py &> /dev/null; then
        print_status $RED "ERROR: idf.py command not found!"
        print_status $YELLOW "Please ensure ESP-IDF is properly installed and sourced."
        exit 1
    fi
    
    print_status $GREEN "✓ ESP-IDF environment is ready"
    print_status $GREEN "✓ IDF_PATH: $IDF_PATH"
    print_status $GREEN "✓ Using $PARALLEL_JOBS parallel jobs"
}

# Function to find all example projects
find_examples() {
    print_header "Discovering Example Projects"
    
    # Find all example directories with CMakeLists.txt (exclude build dirs and submodules)
    local example_dirs=$(find "$SCRIPT_DIR" -path "*/examples/*" -name "CMakeLists.txt" \
        | grep -v "/managed_components/" \
        | grep -v "/build/" \
        | grep -v "/ESP32-HUB75-MatrixPanel-I2S-DMA/" \
        | grep -v "/main/CMakeLists.txt" \
        | xargs dirname)
    
    if [ -z "$example_dirs" ]; then
        print_status $YELLOW "No example projects found!"
        exit 0
    fi
    
    # Store examples in array
    readarray -t EXAMPLES <<< "$example_dirs"
    TOTAL_EXAMPLES=${#EXAMPLES[@]}
    
    print_status $GREEN "Found $TOTAL_EXAMPLES example projects:"
    for example in "${EXAMPLES[@]}"; do
        local rel_path=$(realpath --relative-to="$SCRIPT_DIR" "$example" 2>/dev/null || echo "$example")
        print_status $YELLOW "  - $rel_path"
    done
}

# Function to clean a project
clean_project() {
    local project_dir=$1
    local project_name=$(basename "$project_dir")
    
    if [ -d "$project_dir/$BUILD_DIR_NAME" ]; then
        print_status $YELLOW "  Cleaning existing build directory..."
        rm -rf "$project_dir/$BUILD_DIR_NAME"
    fi
}

# Function to build a single example
build_example() {
    local example_dir=$1
    local example_name=$(basename "$example_dir")
    local component_name=$(basename "$(dirname "$(dirname "$example_dir")")")
    local rel_path=$(realpath --relative-to="$SCRIPT_DIR" "$example_dir" 2>/dev/null || echo "$example_dir")
    
    print_status $BLUE "Building: $component_name/$example_name"
    print_status $YELLOW "  Path: $rel_path"
    
    # Change to example directory
    cd "$example_dir"
    
    # Clean if requested
    if [ "$CLEAN_BUILD" = "true" ]; then
        clean_project "$example_dir"
    fi
    
    # Create log file for this build
    local log_file="$SCRIPT_DIR/build_log_${component_name}_${example_name}.txt"
    
    # Build the project
    local start_time=$(date +%s)
    
    # Set parallel jobs through environment variable for CMake/Ninja
    export CMAKE_BUILD_PARALLEL_LEVEL=$PARALLEL_JOBS
    
    if idf.py build > "$log_file" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        print_status $GREEN "  ✓ SUCCESS (${duration}s)"
        SUCCESSFUL_BUILDS=$((SUCCESSFUL_BUILDS + 1))
        SUCCESSFUL_EXAMPLES+=("$rel_path")
        
        # Remove log file on success (unless verbose)
        [ "$VERBOSE" != "true" ] && rm -f "$log_file"
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        print_status $RED "  ✗ FAILED (${duration}s)"
        print_status $RED "    Log: $log_file"
        FAILED_BUILDS=$((FAILED_BUILDS + 1))
        FAILED_EXAMPLES+=("$rel_path")
        
        # Show last few lines of error log
        if [ "$VERBOSE" = "true" ]; then
            print_status $RED "    Last 10 lines of error log:"
            tail -n 10 "$log_file" | sed 's/^/      /'
        fi
    fi
    
    echo
}

# Function to print final summary
print_summary() {
    print_header "Build Summary"
    
    print_status $GREEN "Total Examples: $TOTAL_EXAMPLES"
    print_status $GREEN "Successful: $SUCCESSFUL_BUILDS"
    print_status $RED "Failed: $FAILED_BUILDS"
    
    if [ $SUCCESSFUL_BUILDS -gt 0 ]; then
        echo
        print_status $GREEN "Successful Builds:"
        for example in "${SUCCESSFUL_EXAMPLES[@]}"; do
            print_status $GREEN "  ✓ $example"
        done
    fi
    
    if [ $FAILED_BUILDS -gt 0 ]; then
        echo
        print_status $RED "Failed Builds:"
        for example in "${FAILED_EXAMPLES[@]}"; do
            print_status $RED "  ✗ $example"
        done
        echo
        print_status $YELLOW "Check the log files for detailed error information."
    fi
    
    echo
    if [ $FAILED_BUILDS -eq 0 ]; then
        print_status $GREEN "🎉 All examples built successfully!"
        exit 0
    else
        print_status $RED "❌ $FAILED_BUILDS example(s) failed to build."
        exit 1
    fi
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Options:"
    echo "  -c, --clean     Clean build directories before building"
    echo "  -v, --verbose   Show verbose output including error logs"
    echo "  -j, --jobs N    Number of parallel jobs (default: $PARALLEL_JOBS)"
    echo "  -h, --help      Show this help message"
    echo
    echo "Environment Variables:"
    echo "  PARALLEL_JOBS   Number of parallel jobs for compilation"
    echo
    echo "Examples:"
    echo "  $0                    # Build all examples"
    echo "  $0 --clean           # Clean and build all examples"
    echo "  $0 --verbose         # Build with verbose output"
    echo "  $0 --jobs 8          # Use 8 parallel jobs"
}

# Parse command line arguments
CLEAN_BUILD=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            print_status $RED "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Main execution
main() {
    print_header "ESP-IDF Components - Build All Examples"
    print_status $BLUE "Script: $(basename "$0")"
    print_status $BLUE "Working Directory: $SCRIPT_DIR"
    
    check_prerequisites
    find_examples
    
    print_header "Building Examples"
    
    # Save current directory
    local original_dir=$(pwd)
    
    # Build each example
    for example_dir in "${EXAMPLES[@]}"; do
        build_example "$example_dir"
    done
    
    # Return to original directory
    cd "$original_dir"
    
    # Clean up empty log files
    find "$SCRIPT_DIR" -name "build_log_*.txt" -empty -delete 2>/dev/null || true
    
    print_summary
}

# Run main function
main "$@"
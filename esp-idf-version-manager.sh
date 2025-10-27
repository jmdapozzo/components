#!/bin/bash

# ESP-IDF Version Manager
# This script helps you switch between different ESP-IDF versions

ESP_BASE_DIR="$HOME/esp"
AVAILABLE_VERSIONS=()

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_colored() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to find available ESP-IDF versions
find_versions() {
    if [ ! -d "$ESP_BASE_DIR" ]; then
        print_colored $RED "ESP-IDF base directory not found: $ESP_BASE_DIR"
        exit 1
    fi
    
    AVAILABLE_VERSIONS=()
    for version_dir in "$ESP_BASE_DIR"/v*/esp-idf; do
        if [ -d "$version_dir" ]; then
            version=$(basename "$(dirname "$version_dir")")
            AVAILABLE_VERSIONS+=("$version")
        fi
    done
    
    if [ ${#AVAILABLE_VERSIONS[@]} -eq 0 ]; then
        print_colored $RED "No ESP-IDF versions found in $ESP_BASE_DIR"
        exit 1
    fi
}

# Function to show current version
show_current() {
    if [ -n "$IDF_PATH" ]; then
        if [ -d "$IDF_PATH" ]; then
            current_version=$(basename "$(dirname "$IDF_PATH")")
            print_colored $GREEN "Current ESP-IDF version: $current_version"
            print_colored $BLUE "Path: $IDF_PATH"
            
            # Try to get version info
            if command -v idf.py &> /dev/null; then
                version_info=$(idf.py --version 2>/dev/null || echo "Unknown")
                print_colored $BLUE "Version info: $version_info"
            else
                print_colored $YELLOW "ESP-IDF environment not active (idf.py not found)"
            fi
        else
            print_colored $RED "IDF_PATH points to non-existent directory: $IDF_PATH"
        fi
    else
        print_colored $RED "IDF_PATH not set"
    fi
}

# Function to list available versions
list_versions() {
    print_colored $BLUE "Available ESP-IDF versions:"
    for version in "${AVAILABLE_VERSIONS[@]}"; do
        version_path="$ESP_BASE_DIR/$version/esp-idf"
        if [ -n "$IDF_PATH" ] && [ "$IDF_PATH" = "$version_path" ]; then
            print_colored $GREEN "  * $version (current)"
        else
            print_colored $YELLOW "    $version"
        fi
    done
}

# Function to update .zshrc
update_zshrc() {
    local new_version=$1
    local zshrc_path="$HOME/.zshrc"
    
    if [ ! -f "$zshrc_path" ]; then
        print_colored $RED ".zshrc file not found: $zshrc_path"
        return 1
    fi
    
    # Create backup
    cp "$zshrc_path" "$zshrc_path.backup.$(date +%Y%m%d_%H%M%S)"
    
    # Update IDF_PATH
    if grep -q "export IDF_PATH=" "$zshrc_path"; then
        sed -i '' "s|export IDF_PATH=.*|export IDF_PATH=~/esp/$new_version/esp-idf|g" "$zshrc_path"
        print_colored $GREEN "Updated IDF_PATH in .zshrc to version $new_version"
    else
        echo "export IDF_PATH=~/esp/$new_version/esp-idf" >> "$zshrc_path"
        print_colored $GREEN "Added IDF_PATH to .zshrc for version $new_version"
    fi
    
    return 0
}

# Function to switch version
switch_version() {
    local target_version=$1
    
    if [[ ! " ${AVAILABLE_VERSIONS[@]} " =~ " ${target_version} " ]]; then
        print_colored $RED "Version $target_version not found!"
        print_colored $YELLOW "Available versions:"
        for version in "${AVAILABLE_VERSIONS[@]}"; do
            echo "  - $version"
        done
        exit 1
    fi
    
    local target_path="$ESP_BASE_DIR/$target_version/esp-idf"
    
    # Check if version exists
    if [ ! -d "$target_path" ]; then
        print_colored $RED "ESP-IDF directory not found: $target_path"
        exit 1
    fi
    
    # Update .zshrc
    if update_zshrc "$target_version"; then
        print_colored $GREEN "Successfully switched to ESP-IDF $target_version"
        print_colored $YELLOW "Please restart your terminal or run:"
        print_colored $BLUE "  source ~/.zshrc"
        print_colored $BLUE "  . $target_path/export.sh"
    else
        print_colored $RED "Failed to update .zshrc"
        exit 1
    fi
}

# Function to install tools for a version
install_tools() {
    local target_version=$1
    
    if [[ ! " ${AVAILABLE_VERSIONS[@]} " =~ " ${target_version} " ]]; then
        print_colored $RED "Version $target_version not found!"
        exit 1
    fi
    
    local target_path="$ESP_BASE_DIR/$target_version/esp-idf"
    
    print_colored $BLUE "Installing tools for ESP-IDF $target_version..."
    cd "$target_path" && ./install.sh
    
    if [ $? -eq 0 ]; then
        print_colored $GREEN "Tools installed successfully for $target_version"
    else
        print_colored $RED "Failed to install tools for $target_version"
        exit 1
    fi
}

# Function to show usage
show_usage() {
    echo "ESP-IDF Version Manager"
    echo
    echo "Usage: $0 [COMMAND] [VERSION]"
    echo
    echo "Commands:"
    echo "  current              Show current ESP-IDF version"
    echo "  list                 List available ESP-IDF versions"
    echo "  switch <version>     Switch to specified version"
    echo "  install <version>    Install tools for specified version"
    echo "  help                 Show this help message"
    echo
    echo "Examples:"
    echo "  $0 current"
    echo "  $0 list"
    echo "  $0 switch v5.5.1"
    echo "  $0 install v5.5.1"
}

# Main execution
main() {
    find_versions
    
    case "${1:-help}" in
        "current")
            show_current
            ;;
        "list")
            list_versions
            ;;
        "switch")
            if [ -z "$2" ]; then
                print_colored $RED "Please specify a version to switch to"
                echo
                list_versions
                exit 1
            fi
            switch_version "$2"
            ;;
        "install")
            if [ -z "$2" ]; then
                print_colored $RED "Please specify a version to install tools for"
                echo
                list_versions
                exit 1
            fi
            install_tools "$2"
            ;;
        "help"|"-h"|"--help")
            show_usage
            ;;
        *)
            print_colored $RED "Unknown command: $1"
            echo
            show_usage
            exit 1
            ;;
    esac
}

main "$@"
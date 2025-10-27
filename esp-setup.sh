#!/bin/bash

# ESP-IDF Auto Environment Setup
# This script automatically sources the ESP-IDF environment

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_colored() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to setup ESP-IDF environment
setup_esp_idf() {
    # Check if IDF_PATH is set
    if [ -z "$IDF_PATH" ]; then
        print_colored $YELLOW "IDF_PATH not set. Please check your shell configuration."
        return 1
    fi
    
    # Check if ESP-IDF directory exists
    if [ ! -d "$IDF_PATH" ]; then
        print_colored $YELLOW "ESP-IDF directory not found: $IDF_PATH"
        return 1
    fi
    
    # Check if export.sh exists
    if [ ! -f "$IDF_PATH/export.sh" ]; then
        print_colored $YELLOW "ESP-IDF export.sh not found: $IDF_PATH/export.sh"
        return 1
    fi
    
    # Check if already sourced by looking for ESP-IDF Python environment
    if [[ "$PATH" == *".espressif/python_env"* ]]; then
        print_colored $GREEN "ESP-IDF environment already active"
        return 0
    fi
    
    # Source ESP-IDF environment
    print_colored $BLUE "Setting up ESP-IDF environment..."
    cd "$IDF_PATH" && . ./export.sh
    
    if [ $? -eq 0 ]; then
        print_colored $GREEN "ESP-IDF environment activated successfully"
        return 0
    else
        print_colored $YELLOW "Failed to activate ESP-IDF environment"
        return 1
    fi
}

# Function to add alias to shell configuration
add_esp_alias() {
    local shell_config=""
    
    # Determine shell configuration file
    if [ "$SHELL" = "/bin/zsh" ] || [ "$SHELL" = "/usr/bin/zsh" ]; then
        shell_config="$HOME/.zshrc"
    elif [ "$SHELL" = "/bin/bash" ] || [ "$SHELL" = "/usr/bin/bash" ]; then
        shell_config="$HOME/.bashrc"
    else
        print_colored $YELLOW "Unknown shell: $SHELL"
        return 1
    fi
    
    # Check if alias already exists
    if grep -q "alias esp-setup" "$shell_config" 2>/dev/null; then
        print_colored $GREEN "ESP setup alias already exists in $shell_config"
        return 0
    fi
    
    # Add alias
    echo "" >> "$shell_config"
    echo "# ESP-IDF Auto Setup Alias" >> "$shell_config"
    echo "alias esp-setup='source $PWD/esp-setup.sh'" >> "$shell_config"
    
    print_colored $GREEN "Added 'esp-setup' alias to $shell_config"
    print_colored $BLUE "Usage: esp-setup (to activate ESP-IDF environment)"
    print_colored $YELLOW "Restart your terminal or run: source $shell_config"
}

# Function to show usage
show_usage() {
    echo "ESP-IDF Auto Environment Setup"
    echo
    echo "Usage: $0 [COMMAND]"
    echo
    echo "Commands:"
    echo "  setup      Setup ESP-IDF environment (default)"
    echo "  install    Install 'esp-setup' alias to shell config"
    echo "  help       Show this help message"
    echo
    echo "Examples:"
    echo "  $0                # Setup ESP-IDF environment"
    echo "  $0 setup          # Setup ESP-IDF environment"
    echo "  $0 install        # Install alias to shell config"
}

# Main execution
case "${1:-setup}" in
    "setup")
        setup_esp_idf
        ;;
    "install")
        add_esp_alias
        ;;
    "help"|"-h"|"--help")
        show_usage
        ;;
    *)
        print_colored $YELLOW "Unknown command: $1"
        echo
        show_usage
        exit 1
        ;;
esac
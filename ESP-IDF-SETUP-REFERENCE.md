# ESP-IDF v5.5.1 Setup - Quick Reference

## 🎯 Current Status
- **ESP-IDF Version**: v5.5.1 ✅
- **Installation Path**: `/Users/jmdapozzo/esp/v5.5.1/esp-idf`
- **Auto-activation**: Enabled ✅
- **Build Environment**: Working ✅

## 🚀 Quick Commands

### Build All Examples
```bash
# Simple build
make build-examples

# Parallel build (faster)
make build-examples-parallel

# Clean and build
make build-examples-clean

# Test single example
make test-build

# Check environment
make check-env
```

### ESP-IDF Version Management
```bash
# Check current version
./esp-idf-version-manager.sh current

# List available versions
./esp-idf-version-manager.sh list

# Switch to different version
./esp-idf-version-manager.sh switch v5.4.1
./esp-idf-version-manager.sh switch v5.5.1
```

### Manual ESP-IDF Setup (if needed)
```bash
# Manual activation (usually not needed)
source ./esp-setup.sh

# Or directly
. /Users/jmdapozzo/esp/v5.5.1/esp-idf/export.sh
```

## 🔧 What Was Fixed

### 1. **Environment Issue**
- **Problem**: `No module named 'yaml'` error
- **Cause**: ESP-IDF Python virtual environment not activated
- **Solution**: Sourced ESP-IDF environment with `. ./export.sh`

### 2. **Auto-activation Setup**
- **Added to `.zshrc`**: Automatic ESP-IDF environment activation
- **Benefits**: No need to manually source environment in new terminals

### 3. **Version Management**
- **Updated IDF_PATH**: From v5.4.1 to v5.5.1
- **Created version manager**: Easy switching between ESP-IDF versions
- **Backup created**: `.zshrc.backup` for safety

## 📁 Available Scripts

### Build Scripts
- `build_all_examples.sh` - Bash script for building all examples
- `build_all_examples.py` - Python script with advanced features
- `Makefile` - Convenient shortcuts for common tasks

### Management Scripts
- `esp-idf-version-manager.sh` - Switch between ESP-IDF versions
- `esp-setup.sh` - Manual ESP-IDF environment setup

## 🐛 Troubleshooting

### If Environment Not Working
```bash
# Check current setup
make check-env

# Manual activation
source ./esp-setup.sh

# Restart terminal and try again
```

### If Build Fails with "-j" Error
```bash
# Error: "No such option: -j"
# This was fixed in ESP-IDF v5.5.1 - parallel jobs are now controlled via CMAKE_BUILD_PARALLEL_LEVEL
# Our scripts have been updated to handle this correctly
```

### If Build Fails
```bash
# Clean all builds (important after version change)
find . -name "build" -type d -exec rm -rf {} +
find . -name "managed_components" -type d -exec rm -rf {} +

# Test single example
make test-build
```

### If Wrong ESP-IDF Version
```bash
# Check current version
./esp-idf-version-manager.sh current

# Switch version if needed
./esp-idf-version-manager.sh switch v5.5.1
```

## 🎯 Next Steps

1. **Start new terminal** - ESP-IDF v5.5.1 will activate automatically
2. **Build your components** - Use `make build-examples`
3. **Update component dependencies** - Some may need v5.5.1 updates
4. **Test thoroughly** - Verify all examples work with new version

## 📝 Notes

- **Configuration changes**: Some sdkconfig options changed between v5.4.1 and v5.5.1
- **Dependency updates**: Component manager may suggest dependency updates
- **Python environment**: ESP-IDF v5.5.1 uses its own Python virtual environment
- **Build compatibility**: Projects built with v5.4.1 must be cleaned before building with v5.5.1

## 🔄 Reverting (if needed)

To go back to v5.4.1:
```bash
./esp-idf-version-manager.sh switch v5.4.1
# Restart terminal
make check-env
```

Your ESP-IDF v5.5.1 setup is now complete and working! 🎉
# ESP-IDF Components Build Scripts

This repository includes comprehensive build scripts to compile all example projects across all ESP-IDF components.

## 📁 Available Scripts

### 1. **bash Script** - `build_all_examples.sh`
- **Platform**: Linux/macOS (bash required)
- **Features**: Fast, simple, reliable
- **Best for**: Quick builds and CI/CD

### 2. **Python Script** - `build_all_examples.py`
- **Platform**: Cross-platform (Python 3.6+)
- **Features**: Advanced features, parallel builds, reports
- **Best for**: Development and detailed analysis

### 3. **Makefile** - `Makefile`
- **Platform**: Linux/macOS (make required)
- **Features**: Convenient shortcuts
- **Best for**: Easy access to common build tasks

## 🚀 Quick Start

### Prerequisites
```bash
# Source ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Verify environment
echo $IDF_PATH
idf.py --version
```

### Simple Usage
```bash
# Using Makefile (easiest)
make build-examples

# Using bash script directly
./build_all_examples.sh

# Using Python script directly  
./build_all_examples.py
```

## 📋 Script Features Comparison

| Feature | Bash Script | Python Script | Makefile |
|---------|-------------|---------------|----------|
| Sequential build | ✅ | ✅ | ✅ |
| Parallel compilation | ✅ | ✅ | ✅ |
| Parallel project builds | ❌ | ✅ | ✅ |
| Progress indicators | ✅ | ✅ | ✅ |
| Error logging | ✅ | ✅ | ✅ |
| Build reports | ❌ | ✅ (JSON/HTML) | ✅ |
| Timeout handling | ❌ | ✅ | ❌ |
| Cross-platform | Linux/macOS | All platforms | Linux/macOS |

## 🔧 Advanced Usage

### Bash Script Options
```bash
./build_all_examples.sh [OPTIONS]

Options:
  -c, --clean     Clean build directories before building
  -v, --verbose   Show verbose output including error logs
  -j, --jobs N    Number of parallel jobs (default: auto-detect)
  -h, --help      Show help message

Examples:
  ./build_all_examples.sh                    # Build all examples
  ./build_all_examples.sh --clean           # Clean and build
  ./build_all_examples.sh --verbose         # Verbose output
  ./build_all_examples.sh --jobs 8          # Use 8 parallel jobs
```

### Python Script Options
```bash
./build_all_examples.py [OPTIONS]

Options:
  -c, --clean           Clean build directories before building
  -v, --verbose         Show verbose output including error logs
  -j, --jobs N          Number of parallel compilation jobs
  -p, --parallel N      Number of concurrent project builds
  -t, --timeout N       Timeout per build in seconds (default: 300)
  --json-report         Generate JSON build report
  --html-report         Generate HTML build report

Examples:
  ./build_all_examples.py                          # Sequential build
  ./build_all_examples.py --parallel 4             # 4 concurrent builds
  ./build_all_examples.py --clean --verbose        # Clean + verbose
  ./build_all_examples.py --json-report --html-report  # With reports
  ./build_all_examples.py --parallel 2 --jobs 8 --timeout 600  # Full options
```

### Makefile Targets
```bash
make [TARGET] [OPTIONS]

Targets:
  help                   Show this help
  build-examples         Build all examples sequentially
  build-examples-clean   Clean and build all examples
  build-examples-parallel Build examples in parallel
  build-fast             Fast parallel build with minimal output
  test-build             Test build one example to verify setup
  info                   Show example information
  clean-logs             Clean all build log files
  reports                Generate build reports
  check-env              Check ESP-IDF environment

Options:
  JOBS=N                 Number of compilation jobs
  PARALLEL=N             Number of concurrent builds (Python script)
  TIMEOUT=N              Build timeout in seconds (Python script)

Examples:
  make build-examples                          # Basic build
  make build-examples-parallel PARALLEL=2     # Parallel build
  make build-fast JOBS=8 PARALLEL=4          # Fast build
  make reports PARALLEL=3                     # Generate reports
```

## 📊 Build Reports (Python Script Only)

The Python script can generate detailed build reports:

### JSON Report (`build_report.json`)
```json
{
  "timestamp": "2025-10-27 10:30:00",
  "total_examples": 12,
  "successful": 10,
  "failed": 2,
  "total_duration": 245.6,
  "results": [...]
}
```

### HTML Report (`build_report.html`)
- Visual summary with colors
- Sortable table of results
- Build duration analysis
- Error details

## 🎯 Performance Optimization

### For Fast Builds
```bash
# Maximum parallelism
make build-fast JOBS=8 PARALLEL=4

# Or directly with Python script
./build_all_examples.py --parallel 4 --jobs 8 --timeout 180
```

### For Large Projects
```bash
# Increase timeout and reduce concurrency
./build_all_examples.py --parallel 2 --timeout 600 --verbose
```

### For CI/CD
```bash
# Simple, reliable bash script
./build_all_examples.sh --clean --jobs 4

# Or with reports for analysis
./build_all_examples.py --clean --parallel 2 --json-report
```

## 🐛 Troubleshooting

### Common Issues

#### 1. ESP-IDF Environment Not Set
```bash
ERROR: IDF_PATH environment variable is not set!
```
**Solution:**
```bash
. $HOME/esp/esp-idf/export.sh
```

#### 2. Build Failures
- Check individual log files: `build_log_<component>_<example>.txt`
- Use verbose mode: `--verbose`
- Try building individual examples manually

#### 3. Timeout Issues
```bash
# Increase timeout (Python script only)
./build_all_examples.py --timeout 600  # 10 minutes
```

#### 4. Memory Issues with Parallel Builds
```bash
# Reduce concurrent builds
./build_all_examples.py --parallel 1 --jobs 4
```

### Debugging Steps

1. **Test Environment**:
   ```bash
   make check-env
   make test-build
   ```

2. **Check Examples**:
   ```bash
   make info
   ```

3. **Build Single Example**:
   ```bash
   cd digitalInput/examples/basic
   idf.py build
   ```

4. **Clean Everything**:
   ```bash
   make clean-logs
   find . -name "build" -type d -exec rm -rf {} +
   ```

## 📁 Generated Files

### Log Files
- `build_log_<component>_<example>.txt` - Individual build logs (on failure)

### Report Files (Python script)
- `build_report.json` - JSON format report
- `build_report.html` - HTML format report

### Build Artifacts
- `<example>/build/` - Standard ESP-IDF build directories

## 🔧 Customization

### Adding New Examples
1. Create example directory: `<component>/examples/<example_name>/`
2. Add `CMakeLists.txt` with standard ESP-IDF project structure
3. Scripts will automatically discover and build new examples

### Excluding Examples
The scripts automatically exclude:
- Build directories (`/build/`)
- Managed components (`/managed_components/`)
- Submodule examples (`/ESP32-HUB75-MatrixPanel-I2S-DMA/`)
- Main component CMakeLists (`/main/CMakeLists.txt`)

To manually exclude additional examples, modify the filter patterns in the scripts.

## 📝 Best Practices

1. **Always source ESP-IDF environment first**
2. **Use `make check-env` to verify setup**
3. **Start with `make test-build` for new setups**
4. **Use parallel builds for development**
5. **Use clean builds for releases**
6. **Generate reports for build analysis**
7. **Keep build logs for failed builds**

## 🤝 Integration

### CI/CD Integration
```bash
# In your CI pipeline
source $HOME/esp/esp-idf/export.sh
make build-examples-clean JOBS=4
```

### Development Workflow
```bash
# Daily development
make build-examples-parallel PARALLEL=4

# Before commits
make build-examples-clean

# Analysis
make reports
```

## 📈 Example Output

```bash
$ make build-examples-parallel

============================================
ESP-IDF Components - Build All Examples
============================================

✓ ESP-IDF environment is ready
✓ Found 6 example projects:
  - digitalInput/examples/basic
  - gps/examples/basic
  - ledMatrix/examples/x64y32
  - ledPanel/examples/max7219
  - ledPanel/examples/mbi5026
  - statusLed/examples/basic

Building: digitalInput/basic
  ✓ SUCCESS (23.4s)

Building: statusLed/basic  
  ✓ SUCCESS (18.7s)

...

============================================
Build Summary
============================================
Total Examples: 6
Successful: 6
Failed: 0
🎉 All examples built successfully!
```
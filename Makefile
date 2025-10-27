# ESP-IDF Components - Build Scripts Makefile
# Provides convenient targets for building all examples

.PHONY: help build-examples build-examples-clean build-examples-parallel test-build info

# Default target
help:
	@echo "ESP-IDF Components Build Scripts"
	@echo "================================"
	@echo ""
	@echo "Available targets:"
	@echo "  build-examples        - Build all examples sequentially"
	@echo "  build-examples-clean  - Clean and build all examples"
	@echo "  build-examples-parallel - Build examples in parallel (4 concurrent)"
	@echo "  build-fast            - Fast parallel build with minimal output"
	@echo "  test-build            - Test build one example to verify setup"
	@echo "  info                  - Show example information"
	@echo "  clean-logs            - Clean all build log files"
	@echo "  reports               - Generate build reports (requires Python script)"
	@echo ""
	@echo "Script options:"
	@echo "  JOBS=N               - Number of compilation jobs (default: auto-detect)"
	@echo "  PARALLEL=N           - Number of concurrent builds (Python script only)"
	@echo "  TIMEOUT=N            - Build timeout in seconds (Python script only)"
	@echo ""
	@echo "Examples:"
	@echo "  make build-examples"
	@echo "  make build-examples-parallel PARALLEL=2"
	@echo "  make build-examples JOBS=8"

# Build all examples sequentially using bash script
build-examples:
	@echo "Building all examples sequentially..."
	./build_all_examples.sh $(if $(JOBS),-j $(JOBS))

# Clean and build all examples
build-examples-clean:
	@echo "Cleaning and building all examples..."
	./build_all_examples.sh --clean $(if $(JOBS),-j $(JOBS))

# Build examples in parallel using Python script
build-examples-parallel:
	@echo "Building all examples in parallel..."
	./build_all_examples.py --parallel $(or $(PARALLEL),4) $(if $(JOBS),-j $(JOBS)) $(if $(TIMEOUT),-t $(TIMEOUT))

# Fast parallel build with minimal output
build-fast:
	@echo "Fast parallel build..."
	./build_all_examples.py --parallel $(or $(PARALLEL),4) $(if $(JOBS),-j $(JOBS),--jobs 8) --timeout $(or $(TIMEOUT),180)

# Test build - build just one example to verify setup
test-build:
	@echo "Testing build environment with one example..."
	@FIRST_EXAMPLE=$$(find . -path "*/examples/*" -name "CMakeLists.txt" | grep -v "/managed_components/" | grep -v "/build/" | grep -v "/ESP32-HUB75-MatrixPanel-I2S-DMA/" | grep -v "/main/CMakeLists.txt" | head -1 | xargs dirname); \
	if [ -n "$$FIRST_EXAMPLE" ]; then \
		echo "Building: $$FIRST_EXAMPLE"; \
		cd "$$FIRST_EXAMPLE" && idf.py build; \
	else \
		echo "No examples found!"; \
		exit 1; \
	fi

# Show information about examples
info:
	@echo "ESP-IDF Component Examples Information"
	@echo "====================================="
	@echo ""
	@find . -path "*/examples/*" -name "CMakeLists.txt" | grep -v "/managed_components/" | grep -v "/build/" | grep -v "/ESP32-HUB75-MatrixPanel-I2S-DMA/" | grep -v "/main/CMakeLists.txt" | wc -l | xargs echo "Total examples found:"
	@echo ""
	@echo "Examples by component:"
	@find . -path "*/examples/*" -name "CMakeLists.txt" | grep -v "/managed_components/" | grep -v "/build/" | grep -v "/ESP32-HUB75-MatrixPanel-I2S-DMA/" | grep -v "/main/CMakeLists.txt" | xargs dirname | sed 's|./||' | sort | awk -F'/' '{print "  " $$1 "/" $$3}' | sort

# Clean build log files
clean-logs:
	@echo "Cleaning build log files..."
	@rm -f build_log_*.txt
	@rm -f build_report.json build_report.html
	@echo "Cleaned log files."

# Generate detailed reports (Python script only)
reports:
	@echo "Generating build reports..."
	./build_all_examples.py --json-report --html-report $(if $(PARALLEL),--parallel $(PARALLEL)) $(if $(JOBS),-j $(JOBS))

# Check prerequisites
check-env:
	@echo "Checking ESP-IDF environment..."
	@if [ -z "$$IDF_PATH" ]; then \
		echo "ERROR: IDF_PATH not set. Please source ESP-IDF environment."; \
		exit 1; \
	fi
	@if ! command -v idf.py > /dev/null; then \
		echo "ERROR: idf.py not found. Please source ESP-IDF environment."; \
		exit 1; \
	fi
	@echo "ESP-IDF environment OK"
	@echo "IDF_PATH: $$IDF_PATH"

# Show script help
script-help:
	@echo "Bash script help:"
	@echo "=================="
	./build_all_examples.sh --help
	@echo ""
	@echo "Python script help:"
	@echo "==================="
	./build_all_examples.py --help
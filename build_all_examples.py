#!/usr/bin/env python3
"""
ESP-IDF Components - Build All Examples Script (Python Version)

This script compiles all example projects in the components repository.
Provides better cross-platform compatibility and advanced features.
"""

import os
import sys
import subprocess
import argparse
import time
import json
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
import multiprocessing

# ANSI color codes
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    MAGENTA = '\033[0;35m'
    CYAN = '\033[0;36m'
    NC = '\033[0m'  # No Color

def print_colored(message, color=Colors.NC):
    """Print colored message"""
    print(f"{color}{message}{Colors.NC}")

def print_header(message):
    """Print section header"""
    print()
    print_colored("=" * 50, Colors.BLUE)
    print_colored(message, Colors.BLUE)
    print_colored("=" * 50, Colors.BLUE)

class BuildResult:
    """Class to store build results"""
    def __init__(self, example_path, success, duration, log_file=None, error_msg=None):
        self.example_path = example_path
        self.success = success
        self.duration = duration
        self.log_file = log_file
        self.error_msg = error_msg

class ExampleBuilder:
    """Main class for building ESP-IDF examples"""
    
    def __init__(self, args):
        self.args = args
        self.script_dir = Path(__file__).parent.absolute()
        self.examples = []
        self.results = []
        self.start_time = None
        
    def check_prerequisites(self):
        """Check if ESP-IDF environment is properly set up"""
        print_header("Checking Prerequisites")
        
        # Check IDF_PATH
        idf_path = os.environ.get('IDF_PATH')
        if not idf_path:
            print_colored("ERROR: IDF_PATH environment variable is not set!", Colors.RED)
            print_colored("Please source the ESP-IDF environment:", Colors.YELLOW)
            print_colored("  . $HOME/esp/esp-idf/export.sh", Colors.YELLOW)
            sys.exit(1)
        
        # Check idf.py command
        try:
            result = subprocess.run(['idf.py', '--version'], 
                                  capture_output=True, text=True, timeout=10)
            if result.returncode != 0:
                raise subprocess.CalledProcessError(result.returncode, 'idf.py')
        except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
            print_colored("ERROR: idf.py command not found or not working!", Colors.RED)
            print_colored("Please ensure ESP-IDF is properly installed and sourced.", Colors.YELLOW)
            sys.exit(1)
        
        print_colored("✓ ESP-IDF environment is ready", Colors.GREEN)
        print_colored(f"✓ IDF_PATH: {idf_path}", Colors.GREEN)
        print_colored(f"✓ Using {self.args.jobs} parallel jobs", Colors.GREEN)
        
        if self.args.parallel:
            print_colored(f"✓ Parallel building enabled ({self.args.parallel} concurrent builds)", Colors.GREEN)
    
    def find_examples(self):
        """Find all example projects in the repository"""
        print_header("Discovering Example Projects")
        
        # Find all CMakeLists.txt files in examples directories
        example_files = list(self.script_dir.rglob("examples/*/CMakeLists.txt"))
        
        # Filter out unwanted directories
        excluded_patterns = [
            'managed_components',
            'build',
            'ESP32-HUB75-MatrixPanel-I2S-DMA',
            '/main/CMakeLists.txt'
        ]
        
        example_dirs = []
        for cmake_file in example_files:
            cmake_str = str(cmake_file)
            
            # Skip if matches any excluded pattern
            if any(pattern in cmake_str for pattern in excluded_patterns):
                continue
                
            # Skip main/CMakeLists.txt files
            if cmake_file.name == 'CMakeLists.txt' and cmake_file.parent.name == 'main':
                continue
                
            example_dirs.append(cmake_file.parent)
        
        if not example_dirs:
            print_colored("No example projects found!", Colors.YELLOW)
            sys.exit(0)
        
        # Sort examples for consistent output
        self.examples = sorted(example_dirs, key=lambda x: str(x))
        
        print_colored(f"Found {len(self.examples)} example projects:", Colors.GREEN)
        for example in self.examples:
            rel_path = example.relative_to(self.script_dir)
            print_colored(f"  - {rel_path}", Colors.YELLOW)
    
    def clean_project(self, project_dir):
        """Clean build directory of a project"""
        build_dir = project_dir / "build"
        if build_dir.exists():
            print_colored(f"  Cleaning existing build directory...", Colors.YELLOW)
            import shutil
            shutil.rmtree(build_dir)
    
    def build_example(self, example_dir):
        """Build a single example project"""
        example_name = example_dir.name
        component_name = example_dir.parent.parent.name
        rel_path = example_dir.relative_to(self.script_dir)
        
        print_colored(f"Building: {component_name}/{example_name}", Colors.BLUE)
        print_colored(f"  Path: {rel_path}", Colors.YELLOW)
        
        # Clean if requested
        if self.args.clean:
            self.clean_project(example_dir)
        
        # Create log file for this build
        log_file = self.script_dir / f"build_log_{component_name}_{example_name}.txt"
        
        # Build the project
        start_time = time.time()
        
        try:
            cmd = ['idf.py', 'build']
            
            # Set up environment with parallel jobs for CMake/Ninja
            env = os.environ.copy()
            env['CMAKE_BUILD_PARALLEL_LEVEL'] = str(self.args.jobs)
            
            with open(log_file, 'w') as f:
                result = subprocess.run(
                    cmd,
                    cwd=example_dir,
                    stdout=f,
                    stderr=subprocess.STDOUT,
                    env=env,
                    timeout=self.args.timeout
                )
            
            end_time = time.time()
            duration = end_time - start_time
            
            if result.returncode == 0:
                print_colored(f"  ✓ SUCCESS ({duration:.1f}s)", Colors.GREEN)
                
                # Remove log file on success (unless verbose)
                if not self.args.verbose:
                    log_file.unlink(missing_ok=True)
                    log_file = None
                
                return BuildResult(rel_path, True, duration, log_file)
            else:
                print_colored(f"  ✗ FAILED ({duration:.1f}s)", Colors.RED)
                print_colored(f"    Log: {log_file}", Colors.RED)
                
                # Show error details if verbose
                if self.args.verbose:
                    self._show_error_log(log_file)
                
                return BuildResult(rel_path, False, duration, log_file, f"Build failed with exit code {result.returncode}")
                
        except subprocess.TimeoutExpired:
            end_time = time.time()
            duration = end_time - start_time
            
            print_colored(f"  ✗ TIMEOUT ({duration:.1f}s)", Colors.RED)
            print_colored(f"    Log: {log_file}", Colors.RED)
            
            return BuildResult(rel_path, False, duration, log_file, f"Build timeout after {self.args.timeout}s")
            
        except Exception as e:
            end_time = time.time()
            duration = end_time - start_time
            
            print_colored(f"  ✗ ERROR ({duration:.1f}s)", Colors.RED)
            print_colored(f"    Error: {str(e)}", Colors.RED)
            
            return BuildResult(rel_path, False, duration, log_file, str(e))
    
    def _show_error_log(self, log_file):
        """Show last few lines of error log"""
        if log_file and log_file.exists():
            print_colored("    Last 10 lines of error log:", Colors.RED)
            try:
                with open(log_file, 'r') as f:
                    lines = f.readlines()
                    for line in lines[-10:]:
                        print(f"      {line.rstrip()}")
            except Exception:
                print_colored("    Could not read log file", Colors.RED)
    
    def build_all_sequential(self):
        """Build all examples sequentially"""
        print_header("Building Examples (Sequential)")
        
        for i, example_dir in enumerate(self.examples, 1):
            print_colored(f"\n[{i}/{len(self.examples)}]", Colors.CYAN)
            result = self.build_example(example_dir)
            self.results.append(result)
    
    def build_all_parallel(self):
        """Build all examples in parallel"""
        print_header(f"Building Examples (Parallel - {self.args.parallel} concurrent)")
        
        with ThreadPoolExecutor(max_workers=self.args.parallel) as executor:
            # Submit all build jobs
            future_to_example = {
                executor.submit(self.build_example, example_dir): example_dir 
                for example_dir in self.examples
            }
            
            # Process completed builds
            completed = 0
            for future in as_completed(future_to_example):
                completed += 1
                example_dir = future_to_example[future]
                
                try:
                    result = future.result()
                    self.results.append(result)
                    print_colored(f"\n[{completed}/{len(self.examples)}] Completed", Colors.CYAN)
                except Exception as e:
                    rel_path = example_dir.relative_to(self.script_dir)
                    result = BuildResult(rel_path, False, 0, None, str(e))
                    self.results.append(result)
                    print_colored(f"\n[{completed}/{len(self.examples)}] Exception: {e}", Colors.RED)
    
    def generate_report(self):
        """Generate build report"""
        if self.args.json_report:
            self._generate_json_report()
        
        if self.args.html_report:
            self._generate_html_report()
    
    def _generate_json_report(self):
        """Generate JSON build report"""
        report_data = {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "total_examples": len(self.examples),
            "successful": sum(1 for r in self.results if r.success),
            "failed": sum(1 for r in self.results if not r.success),
            "total_duration": sum(r.duration for r in self.results),
            "results": [
                {
                    "example": str(r.example_path),
                    "success": r.success,
                    "duration": r.duration,
                    "log_file": str(r.log_file) if r.log_file else None,
                    "error": r.error_msg
                }
                for r in self.results
            ]
        }
        
        report_file = self.script_dir / "build_report.json"
        with open(report_file, 'w') as f:
            json.dump(report_data, f, indent=2)
        
        print_colored(f"JSON report saved to: {report_file}", Colors.CYAN)
    
    def _generate_html_report(self):
        """Generate HTML build report"""
        successful = [r for r in self.results if r.success]
        failed = [r for r in self.results if not r.success]
        
        html_content = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <title>ESP-IDF Examples Build Report</title>
            <style>
                body {{ font-family: Arial, sans-serif; margin: 20px; }}
                .header {{ background: #f0f0f0; padding: 10px; border-radius: 5px; }}
                .success {{ color: green; }}
                .failure {{ color: red; }}
                .summary {{ background: #e8f4fd; padding: 15px; border-radius: 5px; margin: 20px 0; }}
                table {{ border-collapse: collapse; width: 100%; }}
                th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
                th {{ background-color: #f2f2f2; }}
                .duration {{ text-align: right; }}
            </style>
        </head>
        <body>
            <div class="header">
                <h1>ESP-IDF Examples Build Report</h1>
                <p>Generated: {time.strftime("%Y-%m-%d %H:%M:%S")}</p>
            </div>
            
            <div class="summary">
                <h2>Summary</h2>
                <p><strong>Total Examples:</strong> {len(self.examples)}</p>
                <p><strong class="success">Successful:</strong> {len(successful)}</p>
                <p><strong class="failure">Failed:</strong> {len(failed)}</p>
                <p><strong>Total Duration:</strong> {sum(r.duration for r in self.results):.1f} seconds</p>
            </div>
            
            <h2>Build Results</h2>
            <table>
                <tr>
                    <th>Example</th>
                    <th>Status</th>
                    <th>Duration (s)</th>
                    <th>Error</th>
                </tr>
        """
        
        for result in sorted(self.results, key=lambda x: str(x.example_path)):
            status_class = "success" if result.success else "failure"
            status_text = "✓ SUCCESS" if result.success else "✗ FAILED"
            error_text = result.error_msg or ""
            
            html_content += f"""
                <tr>
                    <td>{result.example_path}</td>
                    <td class="{status_class}">{status_text}</td>
                    <td class="duration">{result.duration:.1f}</td>
                    <td>{error_text}</td>
                </tr>
            """
        
        html_content += """
            </table>
        </body>
        </html>
        """
        
        report_file = self.script_dir / "build_report.html"
        with open(report_file, 'w') as f:
            f.write(html_content)
        
        print_colored(f"HTML report saved to: {report_file}", Colors.CYAN)
    
    def print_summary(self):
        """Print final build summary"""
        print_header("Build Summary")
        
        successful = [r for r in self.results if r.success]
        failed = [r for r in self.results if not r.success]
        total_duration = sum(r.duration for r in self.results)
        
        print_colored(f"Total Examples: {len(self.examples)}", Colors.GREEN)
        print_colored(f"Successful: {len(successful)}", Colors.GREEN)
        print_colored(f"Failed: {len(failed)}", Colors.RED)
        print_colored(f"Total Duration: {total_duration:.1f} seconds", Colors.BLUE)
        
        if self.start_time:
            wall_time = time.time() - self.start_time
            print_colored(f"Wall Clock Time: {wall_time:.1f} seconds", Colors.BLUE)
            if wall_time > 0:
                efficiency = (total_duration / wall_time) * 100
                print_colored(f"Parallelization Efficiency: {efficiency:.1f}%", Colors.BLUE)
        
        if successful:
            print()
            print_colored("Successful Builds:", Colors.GREEN)
            for result in successful:
                print_colored(f"  ✓ {result.example_path} ({result.duration:.1f}s)", Colors.GREEN)
        
        if failed:
            print()
            print_colored("Failed Builds:", Colors.RED)
            for result in failed:
                print_colored(f"  ✗ {result.example_path} ({result.duration:.1f}s)", Colors.RED)
                if result.error_msg:
                    print_colored(f"    Error: {result.error_msg}", Colors.RED)
            
            print()
            print_colored("Check the log files for detailed error information.", Colors.YELLOW)
        
        print()
        if not failed:
            print_colored("🎉 All examples built successfully!", Colors.GREEN)
            return 0
        else:
            print_colored(f"❌ {len(failed)} example(s) failed to build.", Colors.RED)
            return 1
    
    def run(self):
        """Main execution function"""
        print_header("ESP-IDF Components - Build All Examples (Python)")
        print_colored(f"Script: {Path(__file__).name}", Colors.BLUE)
        print_colored(f"Working Directory: {self.script_dir}", Colors.BLUE)
        
        self.start_time = time.time()
        
        self.check_prerequisites()
        self.find_examples()
        
        # Build examples
        if self.args.parallel > 1:
            self.build_all_parallel()
        else:
            self.build_all_sequential()
        
        # Sort results by example path for consistent output
        self.results.sort(key=lambda x: str(x.example_path))
        
        # Generate reports
        self.generate_report()
        
        # Clean up empty log files
        for log_file in self.script_dir.glob("build_log_*.txt"):
            if log_file.stat().st_size == 0:
                log_file.unlink()
        
        return self.print_summary()

def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="Build all ESP-IDF component examples",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                          # Build all examples sequentially
  %(prog)s --clean                  # Clean and build all examples
  %(prog)s --parallel 4             # Build with 4 concurrent builds
  %(prog)s --verbose --json-report  # Verbose with JSON report
  %(prog)s --timeout 600            # Set 10 minute timeout per build
        """
    )
    
    # Default number of CPU cores
    default_jobs = multiprocessing.cpu_count()
    
    parser.add_argument(
        '-c', '--clean',
        action='store_true',
        help='Clean build directories before building'
    )
    
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Show verbose output including error logs'
    )
    
    parser.add_argument(
        '-j', '--jobs',
        type=int,
        default=default_jobs,
        help=f'Number of parallel jobs for compilation (default: {default_jobs})'
    )
    
    parser.add_argument(
        '-p', '--parallel',
        type=int,
        default=1,
        help='Number of concurrent example builds (default: 1, sequential)'
    )
    
    parser.add_argument(
        '-t', '--timeout',
        type=int,
        default=300,
        help='Timeout per build in seconds (default: 300)'
    )
    
    parser.add_argument(
        '--json-report',
        action='store_true',
        help='Generate JSON build report'
    )
    
    parser.add_argument(
        '--html-report',
        action='store_true',
        help='Generate HTML build report'
    )
    
    args = parser.parse_args()
    
    # Validate arguments
    if args.jobs < 1:
        print_colored("Error: Number of jobs must be at least 1", Colors.RED)
        sys.exit(1)
    
    if args.parallel < 1:
        print_colored("Error: Number of parallel builds must be at least 1", Colors.RED)
        sys.exit(1)
    
    if args.timeout < 1:
        print_colored("Error: Timeout must be at least 1 second", Colors.RED)
        sys.exit(1)
    
    # Run the builder
    builder = ExampleBuilder(args)
    return builder.run()

if __name__ == '__main__':
    sys.exit(main())
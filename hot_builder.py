# copile mon code cpp toutes les 30 secondes
# Build : g++ -std=c++20 -o optimizing main.cpp -I/usr/include -lsfml-graphics -lsfml-window -lsfml-system
import os
import time
import subprocess
import sys
import select

# Function to compile the C++ code
def compile_code():
    print("Compiling code...")
    result = subprocess.run(["g++", "-std=c++20", "-o", "optimizing", "main.cpp", "-I/usr/include", "-lsfml-graphics", "-lsfml-window", "-lsfml-system"], capture_output=True, text=True)

    if result.returncode != 0:
        print("Compilation failed:")
        print(result.stderr)
        return False
    else:
        print("Compilation successful.")
        return True

# Function to run the compiled code
def run_code():
    print("Running code...")
    process = subprocess.Popen(["./optimizing"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    try:
        # Wait for the process to complete
        stdout, stderr = process.communicate(timeout=10)
        print(stdout.decode())
        if stderr:
            print("Error:", stderr.decode())
    except subprocess.TimeoutExpired:
        print("Process timed out.")
        process.kill()
        stdout, stderr = process.communicate()
        print(stdout.decode())
        if stderr:
            print("Error:", stderr.decode())
    except Exception as e:
        print(f"An error occurred: {e}")
        process.kill()

# Main function to compile and run the code every 30 seconds and handle Ctrl+C
def main():
    run_enabled = True  # Flag to enable or disable running the code

    try:
        while True:
            os.system('clear' if os.name == 'posix' else 'cls')  # Clear the screen
            if compile_code():
                if run_enabled:
                    run_code()
                else:
                    print("Run mode is disabled. Skipping execution.")
            print("\nPress 'r' to toggle run mode (enabled: {}) or wait 30 seconds...".format(run_enabled))

            for _ in range(30):
                if sys.stdin in select.select([sys.stdin], [], [], 1)[0]:
                    user_input = sys.stdin.read(1).strip().lower()
                    if user_input == 'r':
                        run_enabled = not run_enabled
                        print("Run mode toggled. Now enabled: {}".format(run_enabled))
                        break
    except KeyboardInterrupt:
        print("\nExiting...")
        sys.exit(0)
    except Exception as e:
        print(f"An error occurred: {e}")
        sys.exit(1)
    except SystemExit:
        print("SystemExit")
        sys.exit(0)
    finally:
        print("Cleaning up...")
        # Clean up any resources if necessary
        if os.path.exists("optimizing"):
            os.remove("optimizing")
        print("Done.")

if __name__ == "__main__":
    main()
# This script compiles and runs a C++ program every 30 seconds.
# It handles Ctrl+C to exit gracefully and cleans up resources.
# It also handles compilation errors and process timeouts.

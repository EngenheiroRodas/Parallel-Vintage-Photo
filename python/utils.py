import sys
import subprocess

def get_user_input():
    threads_list = []

    directory = input("Enter the directory path: ")
    if directory == "":
        print("Please enter a directory path", file=sys.stderr)
        exit(1)

    mode = input("Enter the mode (name, size): ")
    if mode not in ["name", "size"]:
        print("Please enter 'name' or 'size'", file=sys.stderr)
        exit(1)
    mode = '-' + mode

    while True:
        temp_thread = input("Enter a number of threads: ")
        if temp_thread == "":
            break
        elif not temp_thread.isdigit():
            print("Please enter a valid number", file=sys.stderr)
            exit(1)
        elif int(temp_thread) <= 0:
            print("Please enter a positive number", file=sys.stderr)
            exit(1)

    # if you type in a number of threads already existing on the array, then that means you're stupid, this is a simple tool

        else:
            threads_list.append(int(temp_thread))

    threads_list.sort()
    return directory, mode, threads_list

def run_process(command):
    process = subprocess.Popen(command)

    process.communicate()

    exit_code = process.returncode
    if exit_code != 0:
        print(f"Command failed with exit code {exit_code}", file=sys.stderr)
        exit(1)
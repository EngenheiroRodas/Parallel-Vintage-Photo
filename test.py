import sys
import subprocess

def run_process(command):
    process = subprocess.Popen(command)

    process.communicate()

    exit_code = process.returncode
    if exit_code != 0:
        print(f"Command failed with exit code {exit_code}", file=sys.stderr)
        exit(1)


command = ["/home/bossman/projects/C/image/photo-old", "datasets/Dataset1", "4", "-name"]
run_process(command)
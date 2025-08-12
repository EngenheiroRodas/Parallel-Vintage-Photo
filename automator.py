from python.utils import get_user_input, run_process

import pandas as pd

directory, mode, threads_list = get_user_input()

for thread in threads_list:
    command = ["./photo-old", directory, str(thread), mode]
    run_process(command)
    print(f"Command executed with {thread} threads", flush=True)

print("All commands executed successfully", flush=True)


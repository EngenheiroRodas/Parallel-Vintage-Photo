### Arguments:
# dataset_dir: The directory containing the .jpeg files
# threads: The number of threads to use for processing (as an array)

import pandas as pd
import sys

from extractor import extract_total

# threads = sys.argv[2:]


if __name__ == "__main__":

    # dataset_dir = sys.argv[1]
    # mode = sys.argv[3]



    dataset_dir = "datasets/Dataset1"
    mode = "-name"
    
    filename = f"{dataset_dir}/timing-1{mode}.txt"

    total = extract_total(filename)

    if total is not None:
        print(f"{filename}: Found total {total}")
    else:
        print(f"{filename}: No total found")
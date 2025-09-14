### Arguments:
# dataset_dir: The directory containing the .jpeg files
# threads: The number of threads to use for processing (as an array)

import pandas as pd
import sys

from extractor import extract_total



if __name__ == "__main__":

    dataset_dir = sys.argv[1]

    mode = sys.argv[2]

    threads = sys.argv[3:]

    total = [None] * len(threads)

    speedup = [None] * len(threads)

    threads = tuple(int(x) for x in threads)

    for i in range(len(threads)):

        filename = f"{dataset_dir}/timing-{threads[i]}{mode}.txt"

        total[i] = extract_total(filename)

        speedup[i] = total[0] / total[i] if total[0] else None

    # Create DataFrame
    df = pd.DataFrame({"threads": threads, "total": total, "speedup": speedup})

    # Write directly to Excel (no extra index column)
    output_file = f"{dataset_dir}/results{mode}.xlsx"
    df.to_excel(output_file, index=False)

    print(f"Results saved to {output_file}")


### Arguments:
# dataset_dir: The directory containing the .jpeg files
# threads: The number of threads to use for processing (as an array)

import sys
from data_to_excel import plot_speedup_time
from extractor import extract_total



if __name__ == "__main__":

    dataset_dir = sys.argv[1]

    mode = sys.argv[2]

    threads = sys.argv[3:]

    thread_time = [None] * len(threads)

    thread_speedup = [None] * len(threads)

    threads = tuple(int(x) for x in threads)

    for i in range(len(threads)):

        filename = f"{dataset_dir}/timing-{threads[i]}{mode}.txt"

        thread_time[i] = extract_total(filename)

        thread_speedup[i] = thread_time[0] / thread_time[i] if thread_time[0] else None

    # Write directly to Excel (no extra index column)
    output_file = f"{dataset_dir}/results{mode}.xlsx"

    plot_speedup_time(dataset_dir, mode, threads, thread_time, thread_speedup, output_file)

    print(f"Excel file with charts saved to {output_file}")



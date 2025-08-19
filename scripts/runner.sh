#! /bin/bash

threads_num=(1 2 4 8 12 16 32) # array of threads to test

read -p "Enter the .jpeg directory: " dataset_dir
if [ -z "$dataset_dir" ]; then # if string is "zero", set default
    dataset_dir="datasets/Dataset1"
fi

read -p "Enter the mode (-size or -name): " mode
if [ -z "$mode" ]; then # if string is "zero", set default
    mode="-name"
fi


for num in "${threads_num[@]}"; do
    echo -e "Starting $dataset_dir work with $num threads and $mode\n"

    ./build/photo-old "$dataset_dir" "$num" "$mode"

    rm -rf "$dataset_dir/old-photos"

    echo -e "\nProcessing completed for $dataset_dir with $num threads and $mode\n\n\n"
done

echo -e "\n\nAll processing completed for $dataset_dir and $mode"

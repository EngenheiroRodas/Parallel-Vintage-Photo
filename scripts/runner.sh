#! /bin/bash

# Include --help menu for user guidance
if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    echo "Usage: ./scripts/runner.sh"
    echo "This script runs the photo-old program with different thread counts and a defined mode."
    echo "You will be prompted to enter the .jpeg directory and mode (-size or -name)."
    echo "If no input is provided, default values will be used."
    echo "After processing, an Excel report will be generated."
    exit 0
fi


threads_num=(1 2 4 8 16) # array of threads to test

read -p "Enter the .jpeg directory: " dataset_dir
if [ -z "$dataset_dir" ]; then # if string is "zero", set default
    dataset_dir="datasets/Dataset1"
fi

read -p "Enter the mode: -size or -name (include the "-"): " mode
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


# Run the Python script to make excel
python3 ./python/__main__.py "$dataset_dir" "$mode" "${threads_num[@]}"

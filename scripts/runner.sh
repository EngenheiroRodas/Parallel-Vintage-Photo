#! /bin/bash

threads_num=[1, 2, 4, 8, 12, 16, 32]

dataset="Dataset2"

mode="-name"

for num in threads_num:
    ./build/photo-old $dataset $num $mode


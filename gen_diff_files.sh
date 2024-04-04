#!/bin/bash

# Run make clean all
make clean all

# Check if the compile executable exists
if [ -f "compile" ]; then
  # Iterate over input files
  for input_file in input*.txt; do
    # Extract suffix from input file name
    suffix="${input_file#input}"

    # Run compile with input and save output to test file
    ./compile < "$input_file" > "test${suffix}"

    # Generate output file name
    output_file="output${suffix}"

    # Compute difference and save to diff file
    diff "$output_file" "test${suffix}" > "diff${suffix}"
  done
else
  echo "Error: compile executable not found."
fi


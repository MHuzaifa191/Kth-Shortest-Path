# Kth Shortest Path

## Overview
This project implements the **Kth Shortest Path** algorithm using parallel computing techniques (OpenMP and MPI). It includes both C and Python implementations and utilizes datasets such as email communication records and character mappings.

## Features
- Parallel computation of path lengths (`length_parallel.c`)
- Parallel computation of paths (`path_parallel.c`)
- Python script to generate and manipulate datasets (`create_data.py`)
- Various datasets related to character mappings and email communication

## Installation
### Prerequisites
- **C Compiler** (e.g., GCC)
- **Python 3.x**
- Required Python libraries (if any)

### Compilation
To compile the C programs:
```bash
gcc -o length_parallel length_parallel.c -fopenmp
gcc -o path_parallel path_parallel.c -fopenmp
```

## Usage
### Running the C Programs
1. Compute path lengths:
   ```bash
   ./length_parallel input_file.txt
   ```
2. Compute the Kth shortest path:
   ```bash
   ./path_parallel input_file.txt k
   ```

### Running the Python Script
To generate data:
```bash
python3 create_data.py
```

## Important Note

- `path_parallel.c` prints the Kth shortest path while the file "length_parallel" prints the 3 shortest node traversal lengths from source to destination. 

- `mapped.txt` is the doctorwho dataset translated into integers using the python code `create_data.py`.

## Dataset Files
- `doctorwho.csv`, `doctorwho1.csv`: Character interaction datasets
- `Enron_Email.txt`: Email communication dataset
- `Path.txt`: Stores computed paths
- `mapped.txt`: Contains mapped data from preprocessing

## Contributing
Feel free to submit pull requests or report issues to enhance this project.

## License
This project is licensed under the MIT License.

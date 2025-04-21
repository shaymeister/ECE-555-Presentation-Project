# Parallel BMP Image Flipping

This project implements a parallel BMP image flipping tool using two models:

- `ImflipMPI`: Uses **MPI** for distributed-memory parallelism.
- `Imflip`: Uses **Pthreads** for shared-memory multithreading.

Both versions support horizontal (`H`) and vertical (`V`) flips on 24-bit uncompressed `.bmp` images.

## Build Instructions

To build everything:

```bash
make
```

Or compile separately:

```bash
# MPI version
make ImflipMPI

# Pthreads version
make Imflip
```

## Usage

### MPI Version

```bash
mpirun -np <num_procs> ./ImflipMPI <input.bmp> <output.bmp> <V|H>
```

- `<num_procs>`: Number of processes
- `V` or `H`: Flip vertically or horizontally

### Pthreads Version

```bash
./Imflip <input.bmp> <output.bmp> <V|H> [num_threads]
```

## Examples

```bash
mpirun -np 4 ./ImflipMPI input.bmp output.bmp V
./Imflip input.bmp output_h.bmp H 8
```

## Output

- Flip type
- Total execution time
- Communication overhead (MPI)
- Flip time (pure operation)

## File List

- `ImflipMPI.c` — MPI version (uses `MPI_Scatterv`, `MPI_Gatherv`, `MPI_Sendrecv`)
- `Imflip.c` — Pthreads version 
- `ImageStuff.c/h` — BMP file I/O
- `Makefile` — Builds both versions

## Notes

- Only supports 24-bit BMP images.
- MPI version uses row-based partitioning across processes.

## Authors

- Diego Perez Sanchez


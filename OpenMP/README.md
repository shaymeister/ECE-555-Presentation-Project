# OpenMP Examples

### Build Instructions
To build everything
```bash
make all
```

or compile separately
```bash
# the main imflip program
make main

# the pi program
make pi
```

## Pi Program
### General
This program implements the estimation of Pi using the following formula:
$$
\pi = \int_0^1{\frac{4}{1 + x^2}}dx
$$

The program implements calculating pi with multiple versions of increasing performance.

### Usage
```bash
./pi <version_number=1|2|3|4> <num_threads>
```

Examples:
```bash
# running pi_v4 with 128 threads
./pi 4 128
```

### Output
Example output of the usage above
```bash
The number of threads that was launched is 128

Pi value: 3.141593

Average Total execution time per REP:  149.8036 ms.  (   1.1703 ms per thread).  

Pi version 4 executed with 128 threads

Performance =  0.150 (ns/step)
```

### Files
- `pi.c` the source code for the program
- `makefile` makefile to compile

## Imflip
### General
This program works the exact same way as the pthreads version discussed in lectures and labs. Instead of using `pthreads` it's using `OpenMP`

### Usage
```bash
./main <input.bmp> <output.bmp> <flip_type=V|H|I|W> <num_threads>
```

Examples:
```bash
# running the vertical flip on the dogL.bmp image, with 16 threads
./main dogL.bmp out.bmp V 16
```

### Output
```bash

   Input BMP File name:             dogL.bmp  (3200 x 2400)

Executing the multi-threaded version with 512 threads ...

The number of threads that was launched is 512

  Output BMP File name:              out.bmp  (3200 x 2400)


Total execution time:    6.0441 ms.  (   0.0118 ms per thread).

Flip Type = 'Vertical (V)'
Performance =  0.787 (ns/pixel)
```

### File List

- `main.c` —  the invoker programs, parse cli input and invoke the proper functions
- `ImageFip.c/h` — Image flipping processing functions
- `ImageStuff.c/h` — BMP file I/O
- `Makefile` — makefile to compile
- `*.bmp` - input/output images


### Notes:
There is no difference between `V` | `W` or `H` | `I` in the case of multithreaded. As for single threaded runs, `W` and `I` uses the openMP wrapped functions to test for overhead compared to normal unwrapped versions of `V` and `H`
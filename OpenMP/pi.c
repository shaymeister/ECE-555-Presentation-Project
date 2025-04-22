#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define REPS 5
#define MAXTHREADS omp_get_max_threads() // Maximum number of threads
#define NUM_STEPS 1000000000             // Number of steps for pi calculation

int nthreads;       // Total number of threads working in parallel
float (*pi_func)(); // Function pointer to calculate pi

float pi_v1() {
  int i;
  double pi = 0.0;
  double x;

  for (i = 0; i < NUM_STEPS; i++) {
    x = (double)i / NUM_STEPS;
    pi += (4.0 / (1.0 + x * x));
  }
  pi /= NUM_STEPS;

  return pi;
}

/**
 * First attemp at parallelizing the pi calculation
 * This version uses OpenMP to parallelize the loop
 * that calculates the value of pi. Each thread calculates
 * a portion of the sum, and then the results are combined
 * at the end.

 * Even though it's parallelized, this version is not memory efficient
 * The sum array is cached and each thread will attempt to write to the same
 * cached memory line. This will cause false sharing and slow down the
 * performance of the program.

 * In fact, the performance is even worse than the serial version.
 */
float pi_v2() {
  double x, pi = 0.0, sum[nthreads];
  double step = 1.0 / (double)NUM_STEPS;

#pragma omp parallel
  {
    int tid = omp_get_thread_num();                 // Thread ID
    int actual_num_threads = omp_get_num_threads(); // Number of threads
    if (tid == 0) {
      nthreads = actual_num_threads; // Total number of threads
    }

    // initialize the sum for each thread
    sum[tid] = 0.0;

    // calculate the chunk size for each thread
    int chunk = NUM_STEPS / actual_num_threads;
    int start = tid * chunk;
    int end = start + chunk - 1;

    // last thread takes the remainder
    if (tid == actual_num_threads - 1) {
      end = NUM_STEPS;
    }

    // calculate the partial sum for each thread
    for (int i = start; i <= end; i++) {
      x = (i + 0.5) * step;
      sum[tid] += 4.0 / (1.0 + x * x);
    }
  }

  // divide by the number of steps to get the final value of pi
  for (int i = 0; i < nthreads; i++) {
    pi += sum[i];
  }
  pi /= NUM_STEPS;

  return pi;
}

float pi_v3() {
  double pi = 0.0;
  double step = 1.0 / (double)NUM_STEPS;

  #pragma omp parallel
  {
    double x, sum;
    int tid = omp_get_thread_num();                 // Thread ID
    int actual_num_threads = omp_get_num_threads(); // Number of threads
    if (tid == 0) {
      nthreads = actual_num_threads; // Total number of threads
    }

    // calculate the chunk size for each thread
    int chunk = NUM_STEPS / actual_num_threads;
    int start = tid * chunk;
    int end = start + chunk - 1;

    // last thread takes the remainder
    if (tid == actual_num_threads - 1) {
      end = NUM_STEPS;
    }

    // calculate the partial sum for each thread
    for (int i = start; i <= end; i++) {
      x = (i + 0.5) * step;
      sum += 4.0 / (1.0 + x * x);
    }

    // only one thread can write to the pi variable at a time
    #pragma omp critical
    {
      pi += sum; // combine the results
    }
  }
  pi /= NUM_STEPS;
  return pi;
}

float pi_v4() {
  double pi = 0.0;
  double step = 1.0 / (double)NUM_STEPS;

#pragma omp parallel for reduction(+ : pi)
  for (int i = 0; i < NUM_STEPS; i++) {
    double x = i * step;
    pi += (4.0 / (1.0 + x * x));
  }
  pi *= step;

  return pi;
}

void pick_pi_function(int version_number) {
  switch (version_number) {
  case 1:
    pi_func = pi_v1;
    break;
  case 2:
    pi_func = pi_v2;
    break;
  case 3:
    pi_func = pi_v3;
    break;
  case 4:
    pi_func = pi_v4;
    break;

  default:
    printf("Invalid version number\n");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  int version_number = 1; // Version of the pi program to use
  double StartTime, EndTime, TimeElapsed;
  struct timeval t;

  // Read in the parameters
  switch (argc) {
  case 1:
    nthreads = 1;
    version_number = 1;
    omp_set_num_threads(nthreads);
    break;
  case 2:
    version_number = atoi(argv[1]);
    nthreads = 1;
    omp_set_num_threads(nthreads);
    break;
  case 3:
    version_number = atoi(argv[1]);
    nthreads = atoi(argv[2]);
    omp_set_num_threads(nthreads);
    break;
  default:
    printf("\n\nUsage: pi [1,2,3,4] [0,1-128]");
    printf("\n\nThe first number is the version to use for the pi program, the "
           "second number is for the number of threads to be used.\n\n");
    printf("\n\nnthreads=0 for the serial version, and 1-128 for the "
           "Pthreads version\n\n");
    printf("\n\nExample: pi\n\n");
    printf("\n\nExample: pi 1\n\n");
    printf("\n\nExample: pi 2 0\n\n");
    printf("\n\nExample: pi 3 8\n\n");
    printf("\n\nNothing executed ... Exiting ...\n\n");
    exit(EXIT_FAILURE);
  }

  pick_pi_function(version_number);

  gettimeofday(&t, NULL);
  StartTime = (double)t.tv_sec * 1000000.0 + ((double)t.tv_usec);

  double pi = 0.0;
  for (int rep = 0; rep < REPS; rep++) {
    pi = (*pi_func)(); // Call the pi function
  }

  printf("\nThe number of threads that was launched is %d\n", nthreads);
  printf("\nPi value: %f\n", pi);

  gettimeofday(&t, NULL);
  EndTime = (double)t.tv_sec * 1000000.0 + ((double)t.tv_usec);
  TimeElapsed = (EndTime - StartTime) / 1000.00;
  TimeElapsed /= (double)REPS;

  printf("\nAverage Total execution time per REP: %9.4f ms.  ", TimeElapsed);
  if (nthreads > 1)
    printf("(%9.4f ms per thread).  ", TimeElapsed / (double)nthreads);
  printf("\n\nPi version %d executed with %d threads\n", version_number,
         nthreads);
  printf("\nPerformance = %6.3f (ns/step)\n",
         1000000 * TimeElapsed / NUM_STEPS);

  return EXIT_SUCCESS;
}
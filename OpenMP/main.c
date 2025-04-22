/******************************************************************************
 * DESCRIPTION:
 *   OpenMP Example - image flip
 *   This program flips a BMP image either vertically or horizontally.
 *   The program uses OpenMP to parallelize the flipping process.
 *   The image is read from a BMP file, flipped, and then written to a new BMP
 *file. The program supports both single-threaded and multi-threaded execution.
 ******************************************************************************/
#include <ctype.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "ImageFlip.h"

#define REPS 129 // needs to be odd, this is to keep the result consistent
#define MAXTHREADS omp_get_max_threads()

struct ImgProp ip;
void (*FlipFunc)(unsigned char **img); // Function pointer to flip the image

unsigned char **TheImage; // This is the main image

void PickFlipFunctionSingleThread(char flipType) {
  switch (flipType) {
  case 'V':
    FlipFunc = FlipVertical;
    break;
  case 'H':
    FlipFunc = FlipHorizontal;
    break;
  case 'W':
    FlipFunc = FlipVerticalMultiThreaded;
    break;
  case 'I':
    FlipFunc = FlipHorizontalMultiThreaded;
    break;
  default:
    printf("\n\nInvalid flip type ... Exiting ...\n\n");
    exit(EXIT_FAILURE);
  }
}

/**
 * PickFlipFunctionMultiThread - Selects the appropriate flip function
 * for multi-threaded execution based on the specified flip type.
 * This function sets the global FlipFunc pointer to the appropriate
 * function for flipping the image.
 *
 * @param flipType: The type of flip to perform ('V', 'H', 'W', 'I').
 */
void PickFlipFunctionMultiThread(char flipType) {
  switch (flipType) {
  case 'V':
  case 'W':
    FlipFunc = FlipVerticalMultiThreaded;
    break;
  case 'H':
  case 'I':
    FlipFunc = FlipHorizontalMultiThreaded;
    break;
  default:
    printf("\n\nInvalid flip type ... Exiting ...\n\n");
    exit(EXIT_FAILURE);
  }
}

char *flipTypeToString(char flipType) {
  switch (flipType) {
  case 'V':
  case 'W':
    return "Vertical (V)";
  case 'H':
  case 'I':
    return "Horizontal (H)";
  default:
    return "Unknown";
  }
}

int main(int argc, char **argv) {
  long nthreads; // Total number of threads working in parallel
  char flipType; // flipType type: V, H, W, I
  struct timeval t;
  double StartTime, EndTime, TimeElapsed;

  // Read in the parameters
  switch (argc) {
  case 3:
    nthreads = 1;
    omp_set_num_threads(nthreads);
    flipType = 'V';
    break;
  case 4:
    nthreads = 1;
    omp_set_num_threads(nthreads);
    flipType = toupper(argv[3][0]);
    break;
  case 5:
    nthreads = atoi(argv[4]);
    omp_set_num_threads(nthreads);
    flipType = toupper(argv[3][0]);
    break;
  default:
    printf("\n\nUsage: imflipPM input output [v,h,w,i] [0,1-128]");
    printf("\n\nUse 'V', 'H' for regular, and 'W', 'I' for the memory-friendly "
           "version of the program\n\n");
    printf("\n\nnthreads=0 for the serial version, and 1-128 for the "
           "Pthreads version\n\n");
    printf("\n\nExample: imflipPM infilename.bmp outname.bmp w 8\n\n");
    printf("\n\nExample: imflipPM infilename.bmp outname.bmp V 0\n\n");
    printf("\n\nNothing executed ... Exiting ...\n\n");
    exit(EXIT_FAILURE);
  }

  // Usage hints
  if ((nthreads < 0) || (nthreads > MAXTHREADS)) {
    printf("\nNumber of threads must be between 0 and %u... \n", MAXTHREADS);
    printf("\n'1' means Pthreads version with a single thread\n");
    printf("\nYou can also specify '0' which means the 'serial' "
           "version... \n\n");
    printf("\n\nNothing executed ... Exiting ...\n\n");
    exit(EXIT_FAILURE);
  }

  TheImage = ReadBMP(argv[1]);
  if (TheImage == NULL) {
    printf("\n\nError reading the input image ... Exiting ...\n\n");
    exit(EXIT_FAILURE);
  }

  if (nthreads == 0 || nthreads == 1) {
    printf("\nExecuting the serial version ...\n");
    PickFlipFunctionSingleThread(flipType);
  } else {
    printf("\nExecuting the multi-threaded version with %li threads ...\n",
           nthreads);
    PickFlipFunctionMultiThread(flipType);
  }

  gettimeofday(&t, NULL);
  StartTime = (double)t.tv_sec * 1000000.0 + ((double)t.tv_usec);

  for (int a = 0; a < REPS; a++) {
    (*FlipFunc)(TheImage);
  }

  printf("\nThe number of threads that was launched is %li\n", nthreads);

  gettimeofday(&t, NULL);
  EndTime = (double)t.tv_sec * 1000000.0 + ((double)t.tv_usec);
  TimeElapsed = (EndTime - StartTime) / 1000.00;
  TimeElapsed /= (double)REPS;

  // merge with header and write to file
  WriteBMP(TheImage, argv[2]);

  // free() the allocated memory for the image
  for (int i = 0; i < ip.Vpixels; i++) {
    free(TheImage[i]);
  }
  free(TheImage);

  printf("\n\nTotal execution time: %9.4f ms.  ", TimeElapsed);
  if (nthreads > 1)
    printf("(%9.4f ms per thread).  ", TimeElapsed / (double)nthreads);
  printf("\n\nFlip Type = '%s'", flipTypeToString(flipType));
  printf("\nPerformance = %6.3f (ns/pixel)\n",
         1000000 * TimeElapsed / (double)(ip.Hpixels * ip.Vpixels));

  return EXIT_SUCCESS;
}
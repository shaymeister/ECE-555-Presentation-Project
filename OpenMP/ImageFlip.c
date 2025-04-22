#include "ImageFlip.h"
#include <omp.h>
#include <string.h>

void FlipVertical(unsigned char **img) {
  struct Pixel pix; // temp swap pixel
  unsigned long col;
  int row;

  // vertical flip
  for (col = 0; col < ip.Hbytes; col += 3) {
    row = 0;
    while (row < ip.Vpixels / 2) {
      pix.B = img[row][col];
      pix.G = img[row][col + 1];
      pix.R = img[row][col + 2];

      img[row][col] = img[ip.Vpixels - (row + 1)][col];
      img[row][col + 1] = img[ip.Vpixels - (row + 1)][col + 1];
      img[row][col + 2] = img[ip.Vpixels - (row + 1)][col + 2];

      img[ip.Vpixels - (row + 1)][col] = pix.B;
      img[ip.Vpixels - (row + 1)][col + 1] = pix.G;
      img[ip.Vpixels - (row + 1)][col + 2] = pix.R;

      row++;
    }
  }
}

void FlipHorizontal(unsigned char **img) {
  struct Pixel pix; // temp swap pixel
  int row, col;

  // horizontal flip
  for (row = 0; row < ip.Vpixels; row++) {
    col = 0;
    while (col < (ip.Hpixels * 3) / 2) {
      pix.B = img[row][col];
      pix.G = img[row][col + 1];
      pix.R = img[row][col + 2];

      img[row][col] = img[row][ip.Hpixels * 3 - (col + 3)];
      img[row][col + 1] = img[row][ip.Hpixels * 3 - (col + 2)];
      img[row][col + 2] = img[row][ip.Hpixels * 3 - (col + 1)];

      img[row][ip.Hpixels * 3 - (col + 3)] = pix.B;
      img[row][ip.Hpixels * 3 - (col + 2)] = pix.G;
      img[row][ip.Hpixels * 3 - (col + 1)] = pix.R;

      col += 3;
    }
  }
}

void FlipVerticalMultiThreaded(unsigned char **img) {
  int row, row2;

  unsigned char Buffer[16384];  // This is the buffer to get the first row
  unsigned char Buffer2[16384]; // This is the buffer to get the second row

// vertical flip
#pragma omp parallel for private(row, row2, Buffer, Buffer2) shared(img)
  for (row = 0; row < ip.Vpixels / 2; row++) {
    memcpy((void *)Buffer, (void *)img[row], (size_t)ip.Hbytes);
    row2 = ip.Vpixels - (row + 1);
    memcpy((void *)Buffer2, (void *)img[row2], (size_t)ip.Hbytes);
    // swap row with row2
    memcpy((void *)img[row], (void *)Buffer2, (size_t)ip.Hbytes);
    memcpy((void *)img[row2], (void *)Buffer, (size_t)ip.Hbytes);
  }
}

void FlipHorizontalMultiThreaded(unsigned char **img) {
  struct Pixel pix; // temp swap pixel
  int row, col;

  unsigned char
      Buffer[16384]; // This is the buffer to use to get the entire row

#pragma omp parallel for private(row, col, pix, Buffer) shared(img)
  for (row = 0; row < ip.Vpixels; row++) {
    memcpy((void *)Buffer, (void *)img[row], (size_t)ip.Hbytes);
    col = 0;
    while (col < ip.Hpixels * 3 / 2) {
      pix.B = Buffer[col];
      pix.G = Buffer[col + 1];
      pix.R = Buffer[col + 2];

      Buffer[col] = Buffer[ip.Hpixels * 3 - (col + 3)];
      Buffer[col + 1] = Buffer[ip.Hpixels * 3 - (col + 2)];
      Buffer[col + 2] = Buffer[ip.Hpixels * 3 - (col + 1)];

      Buffer[ip.Hpixels * 3 - (col + 3)] = pix.B;
      Buffer[ip.Hpixels * 3 - (col + 2)] = pix.G;
      Buffer[ip.Hpixels * 3 - (col + 1)] = pix.R;

      col += 3;
    }
    memcpy((void *)img[row], (void *)Buffer, (size_t)ip.Hbytes);
  }
}

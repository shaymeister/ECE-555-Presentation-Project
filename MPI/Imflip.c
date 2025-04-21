#include <pthread.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include "ImageStuff.h"

#define REPS 	     1
#define MAXTHREADS   128

long  			NumThreads;         		// Total number of threads working in parallel
int 	     	ThParam[MAXTHREADS];		// Thread parameters ...
pthread_t      	ThHandle[MAXTHREADS];		// Thread handles
pthread_attr_t 	ThAttr;						// Pthread attrributes
void (*FlipFunc)(unsigned char* img);		// Function pointer to flip the image
void* (*MTFlipFunc)(void *arg);				// Function pointer to flip the image, multi-threaded version

unsigned char*	TheImage;					// This is the main image
struct ImgProp 	ip;
typedef unsigned char uch;
typedef unsigned long ul;
typedef unsigned int  ui;

#define	IPHB		ip.Hbytes
#define	IPH			ip.Hpixels
#define	IPV			ip.Vpixels
#define	IMAGESIZE	(IPHB*IPV)
#define	IMAGEPIX	(IPH*IPV)

void FlipImageV(unsigned char* img)
{
	struct Pixel pix;
	int row, row2;

	unsigned char* Buffer  = malloc(ip.Hbytes);
	unsigned char* Buffer2 = malloc(ip.Hbytes);

	if (!Buffer || !Buffer2) {
		fprintf(stderr, "Error allocating row buffers\n");
		exit(EXIT_FAILURE);
	}

	int totalRows = ip.Vpixels;
	int rowBytes = ip.Hbytes;

	for (row = 0; row < totalRows / 2; row++) {
		row2 = totalRows - row - 1;

		memcpy(Buffer,  &img[row * rowBytes], rowBytes);
		memcpy(Buffer2, &img[row2 * rowBytes], rowBytes);

		memcpy(&img[row * rowBytes],  Buffer2, rowBytes);
		memcpy(&img[row2 * rowBytes], Buffer,  rowBytes);
	}

	free(Buffer);
	free(Buffer2);
}


void FlipImageH(unsigned char* img)
{
	struct Pixel pix;
	int row, col, opp;

	for(row = 0; row < ip.Vpixels; row++)
	{
		col = 0;
		while(col < (ip.Hpixels * 3) / 2)
		{
			opp = ip.Hpixels * 3 - (col + 3);
			int rowStart = row * ip.Hbytes;

			pix.B = img[rowStart + col];
			pix.G = img[rowStart + col + 1];
			pix.R = img[rowStart + col + 2];

			img[rowStart + col]     = img[rowStart + opp];
			img[rowStart + col + 1] = img[rowStart + opp + 1];
			img[rowStart + col + 2] = img[rowStart + opp + 2];

			img[rowStart + opp]     = pix.B;
			img[rowStart + opp + 1] = pix.G;
			img[rowStart + opp + 2] = pix.R;

			col += 3;
		}
	}
}



void *MTFlipV(void* tid)
{
	struct Pixel pix;
	int row, row2;
	unsigned char* Buffer  = malloc(ip.Hbytes);
	unsigned char* Buffer2 = malloc(ip.Hbytes);

	if (!Buffer || !Buffer2) {
		fprintf(stderr, "Error allocating row buffers\n");
		exit(EXIT_FAILURE);
	}

	long ts = *((int *) tid);
	ts *= ip.Vpixels / NumThreads / 2;
	long te = ts + (ip.Vpixels / NumThreads / 2) - 1;

	for (row = ts; row <= te; row++) {
		row2 = ip.Vpixels - row - 1;

		memcpy(Buffer,  &TheImage[row * ip.Hbytes], ip.Hbytes);
		memcpy(Buffer2, &TheImage[row2 * ip.Hbytes], ip.Hbytes);

		memcpy(&TheImage[row * ip.Hbytes],  Buffer2, ip.Hbytes);
		memcpy(&TheImage[row2 * ip.Hbytes], Buffer,  ip.Hbytes);
	}
	free(Buffer);
	free(Buffer2);
	pthread_exit(NULL);
}


void *MTFlipH(void* tid)
{
	struct Pixel pix;
	int row, col, opp;

	long ts = *((int *) tid);
	ts *= ip.Vpixels / NumThreads;
	long te = ts + ip.Vpixels / NumThreads - 1;

	for(row = ts; row <= te; row++)
	{
		col = 0;
		while(col < (ip.Hpixels * 3) / 2)
		{
			opp = ip.Hpixels * 3 - (col + 3);
			int rowStart = row * ip.Hbytes;

			pix.B = TheImage[rowStart + col];
			pix.G = TheImage[rowStart + col + 1];
			pix.R = TheImage[rowStart + col + 2];

			TheImage[rowStart + col]     = TheImage[rowStart + opp];
			TheImage[rowStart + col + 1] = TheImage[rowStart + opp + 1];
			TheImage[rowStart + col + 2] = TheImage[rowStart + opp + 2];

			TheImage[rowStart + opp]     = pix.B;
			TheImage[rowStart + opp + 1] = pix.G;
			TheImage[rowStart + opp + 2] = pix.R;

			col += 3;
		}
	}
	pthread_exit(NULL);
}

unsigned char *ReadBMPlin(char* fn)
{
	static uch *Img;
	FILE* f = fopen(fn, "rb");
	if (f == NULL){	printf("\n\n%s NOT FOUND\n\n", fn);	exit(EXIT_FAILURE); }

	uch HeaderInfo[54];
	fread(HeaderInfo, sizeof(uch), 54, f); // read the 54-byte header
	// extract image height and width from header
	int width = *(int*)&HeaderInfo[18];			ip.Hpixels = width;
	int height = *(int*)&HeaderInfo[22];		ip.Vpixels = height;
	int RowBytes = (width * 3 + 3) & (~3);		ip.Hbytes = RowBytes;
	//save header for re-use
	memcpy(ip.HeaderInfo, HeaderInfo,54);
	printf("\n Input File name: %17s  (%u x %u)   File Size=%lu", fn, 
			ip.Hpixels, ip.Vpixels, IMAGESIZE);
	// allocate memory to store the main image (1 Dimensional array)
	Img  = (uch *)malloc(IMAGESIZE);
	if (Img == NULL) return Img;      // Cannot allocate memory
	// read the image from disk
	fread(Img, sizeof(uch), IMAGESIZE, f);
	fclose(f);
	return Img;
}
// Write the 1D linear-memory stored image into file.
void WriteBMPlin(uch *Img, char* fn)
{
	FILE* f = fopen(fn, "wb");
	if (f == NULL){ printf("\n\nFILE CREATION ERROR: %s\n\n", fn); exit(1); }
	//write header
	fwrite(ip.HeaderInfo, sizeof(uch), 54, f);
	//write data
	fwrite(Img, sizeof(uch), IMAGESIZE, f);
	printf("\nOutput File name: %17s  (%u x %u)   File Size=%lu\n\n", fn, ip.Hpixels, ip.Vpixels, IMAGESIZE);
	fclose(f);
}

int main(int argc, char** argv)
{
	char 				Flip;
	int 				a,i,ThErr;
	struct timeval 		t;
	double         		StartTime, EndTime;
	double         		TimeElapsed;

	switch (argc){
		case 3 : NumThreads=1; 				Flip = 'V';						break;
		case 4 : NumThreads=1;  			Flip = toupper(argv[3][0]);		break;
		case 5 : NumThreads=atoi(argv[4]);  Flip = toupper(argv[3][0]);		break;
		default: printf("\n\nUsage: imflipP input output [v/h] [thread count]");
		printf("\n\nExample: imflipP infilename.bmp outname.bmp h 8\n\n");
		return 0;
	}
	if((Flip != 'V') && (Flip != 'H') && (Flip != 'G')) {
		printf("Flip option '%c' is invalid. Can only be 'V' , 'H' or 'G' ... Exiting abruptly ...\n",Flip);
		exit(EXIT_FAILURE);
	}

	if((NumThreads<1) || (NumThreads>MAXTHREADS)){
		printf("\nNumber of threads must be between 1 and %u... Exiting abruptly\n",MAXTHREADS);
		exit(EXIT_FAILURE);
	}
	else{
		if(NumThreads != 1){
			printf("\nExecuting the multi-threaded version with %li threads ...\n",NumThreads);
			if (Flip == 'V'){
				MTFlipFunc = MTFlipV;
			}else if(Flip =='H'){
				MTFlipFunc = MTFlipH;
			}
		}
		else{
			printf("\nExecuting the serial version ...\n");
			if (Flip == 'V'){
				FlipFunc = FlipImageV;
			}else if(Flip =='H'){
				FlipFunc = FlipImageH;
			}
		}
	}

	TheImage = ReadBMPlin(argv[1]);

	gettimeofday(&t, NULL);
	StartTime = (double)t.tv_sec*1000000.0 + ((double)t.tv_usec);

	if(NumThreads >1){
		pthread_attr_init(&ThAttr);
		pthread_attr_setdetachstate(&ThAttr, PTHREAD_CREATE_JOINABLE);
		for(a=0; a<REPS; a++){
			for(i=0; i<NumThreads; i++){
				ThParam[i] = i;
				ThErr = pthread_create(&ThHandle[i], &ThAttr, MTFlipFunc, (void *)&ThParam[i]);
				if(ThErr != 0){
					printf("\nThread Creation Error %d. Exiting abruptly... \n",ThErr);
					exit(EXIT_FAILURE);
				}
			}
			pthread_attr_destroy(&ThAttr);
			for(i=0; i<NumThreads; i++){
				pthread_join(ThHandle[i], NULL);
			}
		}
	}else{
		for(a=0; a<REPS; a++){
			(*FlipFunc)(TheImage);
		}
	}

	gettimeofday(&t, NULL);
	EndTime = (double)t.tv_sec*1000000.0 + ((double)t.tv_usec);
	TimeElapsed=(EndTime-StartTime)/1000.00;
	TimeElapsed/=(double)REPS;

	//merge with header and write to file
	WriteBMPlin(TheImage, argv[2]);

	// free() the allocated memory for the image
	free(TheImage);

	printf("\n\nTotal execution time: %9.4f ms (%s)",TimeElapsed, Flip=='V'?"Vertical flip": (Flip == 'H'?"Horizontal flip":"Grayscale") );
	printf(" (%6.3f ns/pixel)\n", 1000000*TimeElapsed/(double)(ip.Hpixels*ip.Vpixels));

	return (EXIT_SUCCESS);
}

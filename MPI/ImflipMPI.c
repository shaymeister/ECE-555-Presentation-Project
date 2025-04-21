#include <mpi.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "ImageStuff.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

typedef unsigned char uch;
typedef unsigned long ul;
typedef unsigned int  ui;

unsigned char* TheImage; // Main image
struct ImgProp ip;

#define	IPHB		ip.Hbytes
#define	IPH			ip.Hpixels
#define	IPV			ip.Vpixels
#define	IMAGESIZE	(IPHB*IPV)
#define	IMAGEPIX	(IPH*IPV)

// To be shared across functions
int rank, numProcs,localRows,rowsPerProc, localSize; 
double sendrecvOH = 0, flipTime = 0; 

//Overhead by sendrcv on Vflip

// Read a 24-bit/pixel BMP file into a 1D linear array.
// Allocate memory to store the 1D image and return its pointer.
uch *ReadBMPlin(char* fn)
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
// Flips image horizontally, swaps pixels on local rows
// No coordination required!
void FlipImageH(unsigned char* img) {
    int row, col;
    struct Pixel pix;
	int mid = ip.Hpixels * 3 / 2; // Middle of the rows
    for (row = 0; row < localRows; row++) {
        int rowStart = row * ip.Hbytes;
        for (col = 0; col < mid; col += 3) {
            int left = rowStart + col;
            int right = rowStart + ip.Hpixels * 3 - (col + 3);

            pix.B = img[left	];
            pix.G = img[left + 1];
            pix.R = img[left + 2];

            img[left	] = img[right	 ];
            img[left + 1] = img[right + 1];
            img[left + 2] = img[right + 2];

            img[right	 ] = pix.B;
            img[right + 1] = pix.G;
            img[right + 2] = pix.R;
        }
    }
}

/*Vertical flipping functions*/

// Helper function to find the rank owning a global row
int getRank(int globalRow) {
    if (numProcs == 1) return 0;
    int lastProcStartRow = (numProcs - 1) * rowsPerProc;
    return (globalRow < lastProcStartRow) ? globalRow / rowsPerProc : numProcs - 1;
}
// Helper function to find the local index within the owner process
int getMyIdx(int globalRow) {
    if (numProcs == 1) return globalRow; 
    return globalRow - rank * rowsPerProc;
}

// Flips Image Vertically , Swaps whole rows
// Each process iterates through half the image, even though they only own a portion
// Requires Coordination between procs !
void FlipImageV(unsigned char *img) {
    int rowSize = ip.Hbytes;
    int halfRows = ip.Vpixels / 2;
    int topOwner, bottomOwner, partnerRank; // Ranks 
    int globalRowIdx, localRowIdx, topLocalIdx, bottomLocalIdx, bottomGlobalIdx; // Indeces
    double start_time, end_time, elapsed_time;
    size_t Offset;

    uch *Buff = (uch *)malloc(rowSize);

    if (!Buff) {
        printf("Rank %d: Failed to allocate row buffer\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return; 
    }

    for (int i = 0; i < halfRows; ++i) { // Iterate trough half of global image
        bottomGlobalIdx = ip.Vpixels - 1 - i; // Opposite idx
        topOwner = getRank(i);
        bottomOwner = getRank(bottomGlobalIdx);

        if (topOwner == bottomOwner) { // Local Swap, no comm
            if (rank == topOwner) {
                topLocalIdx = getMyIdx(i);
                bottomLocalIdx = getMyIdx(bottomGlobalIdx);

                memcpy(Buff, img + (size_t)topLocalIdx * rowSize, rowSize);
                memcpy(img + (size_t)topLocalIdx * rowSize, img + (size_t)bottomLocalIdx * rowSize, rowSize);
                memcpy(img + (size_t)bottomLocalIdx * rowSize, Buff, rowSize);
            }
        } else { // Swap involves 2 procs, comm required
            if (rank == topOwner) { //Determine sendrcv params, based on role
                partnerRank = bottomOwner;
                globalRowIdx = i;
            } else if (rank == bottomOwner) {
                partnerRank = topOwner;
                globalRowIdx = bottomGlobalIdx;
            } else {
                continue; // Not involved in this swap
            }

            localRowIdx = getMyIdx(globalRowIdx);
            Offset = (size_t)localRowIdx * rowSize;
            
            //Swap
            memcpy(Buff, img + Offset, rowSize);
            start_time = MPI_Wtime();
            MPI_Sendrecv(Buff, rowSize, MPI_UNSIGNED_CHAR, partnerRank, 0,
                         img + Offset, rowSize, MPI_UNSIGNED_CHAR, partnerRank, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            end_time = MPI_Wtime();
            elapsed_time = (end_time - start_time) * 1000;

            sendrecvOH += elapsed_time;
        }
    }

    free(Buff);
}


int main(int argc, char** argv) {
    double start_time, end_time, elapsed_time, op_start, op_end, comm_time = 0; //Timing variables
	//MPI initialization
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
	
    //Process commandline arguments
	char InputFileName[255], OutputFileName[255];
	char 				Flip;
    if (argc < 4) {
        if (rank == 0) fprintf(stderr, "Usage: %s <input.bmp> <output.bmp> <Flip>\n", argv[0]);
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }
	strcpy(InputFileName, argv[1]);
	strcpy(OutputFileName, argv[2]);
	Flip = toupper(argv[3][0]);
    if (rank == 0) { //Only rank 0 will read the image
        TheImage = ReadBMPlin(InputFileName);
        start_time = MPI_Wtime(); //Timestamp, program starts
    }

    op_start = MPI_Wtime();
    MPI_Bcast(&ip, sizeof(struct ImgProp), MPI_BYTE, 0, MPI_COMM_WORLD); // Broadcast image properties to all processes
    op_end = MPI_Wtime();
    elapsed_time = (op_end - op_start)*1000;
    comm_time += elapsed_time;

    // How the rows are distributed
    rowsPerProc = ip.Vpixels / numProcs;
    localRows = (rank == numProcs - 1) ? ip.Vpixels - rank * rowsPerProc : rowsPerProc;
    localSize = localRows * ip.Hbytes; //Size of the image portion each rank handles

    // Allocate local buffer
    unsigned char* localImage = (unsigned char*)malloc(localSize);
    if (!localImage) {
        fprintf(stderr, "Memory allocation failed on rank %d\n", rank);
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    // Scatter image data (Distribute)
    int* sendcounts = NULL;
    int* displs = NULL; //displacement (offset)
    if (rank == 0) {
        sendcounts = malloc(numProcs * sizeof(int));
        displs = malloc(numProcs * sizeof(int));
        int offset = 0;
        for (int i = 0; i < numProcs; ++i) {
            int r = (i == numProcs - 1) ? ip.Vpixels - i * rowsPerProc : rowsPerProc;
            sendcounts[i] = r * ip.Hbytes;
            displs[i] = offset;
            offset += sendcounts[i];
        }
    }

    op_start = MPI_Wtime();
    MPI_Scatterv(TheImage, sendcounts, displs, MPI_UNSIGNED_CHAR,
                 localImage, localSize, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    op_end = MPI_Wtime();
    elapsed_time = (op_end - op_start)*1000;
    comm_time += elapsed_time;

    // Perform flipping on assigned rows
    op_start = MPI_Wtime();
    switch(Flip){
		case 'V': FlipImageV(localImage); break;
		case 'H': FlipImageH(localImage); break;
		default: exit(EXIT_FAILURE);
	}
    op_end = MPI_Wtime();
    flipTime = (op_end - op_start) *1000; //How long each rank took to flip

    // Gather results
    op_start = MPI_Wtime();
    MPI_Gatherv(localImage, localSize, MPI_UNSIGNED_CHAR,
                TheImage, sendcounts, displs, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);
    op_end = MPI_Wtime();
    elapsed_time = (op_end - op_start)*1000;

    comm_time += elapsed_time;
    comm_time += sendrecvOH;
    MPI_Barrier(MPI_COMM_WORLD); // Wait for all procs (Sync)
    if (rank == 0) {
        end_time = MPI_Wtime(); //Time stamp, prog ends
        WriteBMPlin(TheImage, OutputFileName);
		elapsed_time = (end_time - start_time)*1000; 
        //Print timing
		printf("\nProgram Executed %c flip and took %f ms. \n",Flip, elapsed_time);
        printf("Total Communication overhead: %f ms\n", comm_time);
        printf("Total \"flipping\" time: %f ms\n",elapsed_time-comm_time);
        free(TheImage); // Free main image
        free(sendcounts);
        free(displs);
    }
    free(localImage); // Each procs frees their local image
    
    MPI_Finalize(); //Prog ends
    return 0;
}

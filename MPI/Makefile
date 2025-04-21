all		: Imflip ImflipMPI

ImflipMPI: 	ImflipMPI.c ImageStuff.c ImageStuff.h
	  		mpicc ImflipMPI.c ImageStuff.c -o ImflipMPI
Imflip 	: Imflip.c  ImageStuff.c ImageStuff.h
	  		gcc Imflip.c ImageStuff.c -o Imflip

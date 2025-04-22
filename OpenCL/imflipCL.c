#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>
#include <ctype.h>
#include <time.h>

unsigned char *TheImg, *CopyImg;                  
unsigned char *GPUImg, *GPUCopyImg, *GPUResult;

struct ImgProp {
    int Hpixels;
    int Vpixels;
    unsigned char HeaderInfo[54];
    unsigned long Hbytes;
} ip;

#define IPHB ip.Hbytes
#define IPH  ip.Hpixels
#define IPV  ip.Vpixels
#define IMAGESIZE (IPHB * IPV)
#define IMAGEPIX (IPH * IPV)

unsigned char *ReadBMPlin(char* fn) {
    //
    // TODO Finish Documentation
    //

    // load image file
    static unsigned char *Img;
    FILE* f = fopen(fn, "rb");
    if (f == NULL) {
        printf("Unable to open file at %s", fn);
        exit(1);
    }

    // read image information
    unsigned char HeaderInfo[54];
    fread(HeaderInfo, sizeof(unsigned char), 54, f);  // Read the 54-byte header
    int width = *(int*)&HeaderInfo[18];  ip.Hpixels = width;
    int height = *(int*)&HeaderInfo[22]; ip.Vpixels = height;
    int RowBytes = (width * 3 + 3) & (~3); ip.Hbytes = RowBytes;
    memcpy(ip.HeaderInfo, HeaderInfo, 54);

    // read image data
    Img = (unsigned char *) malloc(IMAGESIZE);
    if (Img == NULL) {
        printf("Unable to allocate image memory");
        exit(1);
    }    
    fread(Img, sizeof(unsigned char), IMAGESIZE, f);
    fclose(f);

    return Img;
}

void WriteBMPlin(unsigned char *Img, char* fn) {
    //
    // TODO Finish Documentation
    //
    FILE* f = fopen(fn, "wb");
    if (f == NULL) {
        printf("Unable to create file at: %s", fn);
        exit(1);
    }
    fwrite(ip.HeaderInfo, sizeof(unsigned char), 54, f);
    fwrite(Img, sizeof(unsigned char), IMAGESIZE, f);
    fclose(f);
}

cl_program build_program(cl_context context, cl_device_id device, const char *filename) {
    //
    // TODO Finish Documentation
    //

    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Failed to load kernel file!\n");
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    source_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    source_str = (char*)malloc(source_size + 1);
    fread(source_str, 1, source_size, fp);
    source_str[source_size] = '\0';
    fclose(fp);

    cl_int err;
    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &err);
    free(source_str);

    if (err != CL_SUCCESS) {
        printf("Error: Failed to create the program!\n");
        exit(1);
    }

    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        print_build_log(program, device);
        printf("Error: Failed to build program!\n");
        exit(1);
    }

    return program;
}

void print_build_log(cl_program program, cl_device_id device) {
    //
    // TODO Finish Documentation
    //
    size_t log_size;
    cl_int err;

    // Get the size of the build log
    err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to get build log size: %d\n", err);
        return;
    }

    // Allocate memory for the build log
    char *log = (char *) malloc(log_size);
    if (!log) {
        fprintf(stderr, "Failed to allocate memory for build log\n");
        return;
    }

    // Get the build log
    err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to get build log: %d\n", err);
        free(log);
        return;
    }

    // Print the build log
    fprintf(stderr, "Build log:\n%s\n", log);
    free(log);
}

void execute_kernel(cl_command_queue queue, cl_kernel kernel, cl_mem input_img, cl_mem output_img, size_t global_size, size_t local_size) {
    //
    // TODO Finish Documentation
    //
    cl_int err;
    
    size_t global_work_size[2] = {global_size, 1};
    size_t local_work_size[2] = {local_size, 1};

    cl_event event;
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, &event);
    clWaitForEvents(1, &event);

    cl_ulong time_start, time_end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

    printf("Kernel Execution took %f ms\n", (time_end - time_start) / 1e6);

    if (err != CL_SUCCESS) {
        printf("Error: Failed to execute kernel\n");
        exit(1);
    }
    // clFinish(queue);
}

int main(int argc, char **argv) {

    size_t global_size, local_size = 256;
    local_size = 512;
    char kernel_name[256];

    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem input_img_buffer, output_img_buffer;

    // Argument parsing
    if (argc < 5) {
        printf("Usage: %s InputFilename OutputFilename [Kernel Name] [Local Size]\n", argv[0]);
        exit(1);
    }
    char InputFileName[255], OutputFileName[255];
    strcpy(InputFileName, argv[1]);
    strcpy(OutputFileName, argv[2]);
    strncpy(kernel_name, argv[3], sizeof(kernel_name) - 1);
    local_size = atoi(argv[4]);

    printf("Input: %s\nOutput: %s\nKernel: %s\nLocal Size: %d\n",
        InputFileName, OutputFileName, kernel_name, local_size);

    // Read input image
    TheImg = ReadBMPlin(InputFileName);
    if (TheImg == NULL) {
        printf("Cannot allocate memory for the source image!\n");
        exit(1);
    }
    CopyImg = (unsigned char *)malloc(IMAGESIZE);
    if (CopyImg == NULL) {
        free(TheImg);
        printf("Cannot allocate memory for the destination image!\n");
        exit(1);
    }

    // Initialize OpenCL platform and device
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to get platform ID\n");
        exit(1);
    }
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to get device ID\n");
        exit(1);
    }

    // Create context and command queue
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to create OpenCL context\n");
        exit(1);
    }

    queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to create command queue\n");
        exit(1);
    }

    // Build the OpenCL program and use argued kernel
    program = build_program(context, device, "kernels.cl");
    kernel = clCreateKernel(program, kernel_name, &err);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to create kernel\n");
        exit(1);
    }

    clock_t start, end;
    double time_used;
    start = clock();

    // Allocate memory buffers on the device
    input_img_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, IMAGESIZE, TheImg, &err);
    output_img_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, IMAGESIZE, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to create buffer\n");
        exit(1);
    }

    end = clock();
    time_used = ((double) (end - start) / CLOCKS_PER_SEC);
    printf("Memory allocation took %f seconds\n", time_used);

    // Set kernel arguments
    unsigned int h_pixels = IPH;
    unsigned int v_pixels = IPV;
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_img_buffer);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &input_img_buffer);
    err |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &h_pixels);
    err |= clSetKernelArg(kernel, 3, sizeof(unsigned int), &v_pixels);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to set kernel arguments\n");
        exit(1);
    }

    // image size * 3 for the case of the image copy
    global_size = IMAGEPIX * 3;

    start = clock();

    // Execute the kernel
    execute_kernel(queue, kernel, input_img_buffer, output_img_buffer, global_size, local_size);

    end = clock();
    time_used = ((double) (end - start) / CLOCKS_PER_SEC);
    // printf("Kernel Execution took %f seconds\n", time_used);

    start = clock();

    // Read the result back to the CPU
    err = clEnqueueReadBuffer(queue, output_img_buffer, CL_TRUE, 0, IMAGESIZE, CopyImg, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to read buffer. Error code: %d\n", err);
        exit(1);
    }

    end = clock();
    time_used = ((double) (end - start) / CLOCKS_PER_SEC);
    printf("Memory read took %f seconds\n", time_used);

    // Write the result to an output file
    WriteBMPlin(CopyImg, OutputFileName);

    // Clean up OpenCL resources
    clReleaseMemObject(input_img_buffer);
    clReleaseMemObject(output_img_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    // Free CPU memory
    free(TheImg);
    free(CopyImg);

    return 0;
}

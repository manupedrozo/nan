// Lots of code duplication here, but thats on purpose

#include <cuda.h>
#include <nvrtc.h>
#include <vector>
#include <iostream>
#include "cuda_common.h"
#include "cuda_argument_parser.h"
#include "cuda_manager.h"
#include "cuda_compiler.h"

#define NUM_THREADS 128
#define NUM_BLOCKS 32 

const char *KERNEL_NAME     = "saxpy";
const char *KERNEL_FILENAME = "saxpy.cu";
const char *KERNEL_PATH     = "saxpy.cu";
const char *PTX_PATH        = "saxpy";

using namespace cuda_manager;

// Requires compiled ptx
void test_arg_parser() {
  CudaManager cuda_manager;

  // Setup input and output buffers
  size_t n = NUM_THREADS * NUM_BLOCKS;
  size_t buffer_size = n * sizeof(float);

  // Allocate and write buffers
  float *x, *y, *o;
  int xid = cuda_manager.memory_manager.allocate_buffer(buffer_size, (void **) &x);
  int yid = cuda_manager.memory_manager.allocate_buffer(buffer_size, (void **) &y);
  int oid = cuda_manager.memory_manager.allocate_buffer(buffer_size, (void **) &o);
  printf("Allocated buffer x id: %d, ptr: %p\n", xid, x);
  printf("Allocated buffer y id: %d, ptr: %p\n", yid, y);
  printf("Allocated buffer o id: %d, ptr: %p\n", oid, o);

  float a = 2.5f;
  
  for (size_t i = 0; i < n; ++i) {
    x[i] = static_cast<float>(i);
    y[i] = static_cast<float>(i * 2);
  }

  ValueArg<float>  arg_a(a, sizeof(float), true);
  BufferArg arg_x(xid, buffer_size, true);
  BufferArg arg_y(yid, buffer_size, true);
  BufferArg arg_o(oid, buffer_size, false);
  ValueArg<float> arg_n(n, sizeof(float), true);

  std::vector<Arg *> args {&arg_a, &arg_x, &arg_y, &arg_o, &arg_n};

  // Arguments to a string
  int kernel_mem_id = 0; // Not using this, we are manually handling memory
  std::cout << "Arguments to string: \n";
  std::string _arguments = args_to_string(KERNEL_NAME, kernel_mem_id, args);
  const char *arguments = _arguments.c_str();

  // Parse arguments back from the string
  std::vector<Arg *> parsed_args;
  char *kernel_name;
  bool succ = parse_arguments(arguments, parsed_args, &kernel_mem_id, &kernel_name);

  if (!succ) {
    exit(1);
  }

  //std::cout << "Parsed arguments from string kernel [" << kernel_name << "]: \n";
  //print_args(parsed_args);

  // Get ptx and kernel handle
  CudaCompiler cuda_compiler;
  char *ptx = cuda_compiler.read_ptx_from_file(PTX_PATH);

  CUmodule module;
  CUfunction kernel;
  CUDA_SAFE_CALL(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));
  CUDA_SAFE_CALL(cuModuleGetFunction(&kernel, module, kernel_name));

  // Free PTX and kernel_name_
  delete[] kernel_name;
  delete[] ptx;

  // Launch kernel with parsed arguments
  cuda_manager.launch_kernel(kernel, parsed_args, NUM_BLOCKS, NUM_THREADS);

  for (size_t i = 0; i < 10; ++i) { // first 10 results only
    std::cout << a << " * " << x[i] << " + " << y[i] << " = " << o[i] << '\n';
  }

  // Free resources
  for (Arg *arg : parsed_args) { free(arg); }
  CUDA_SAFE_CALL(cuModuleUnload(module));
  delete[] x;
  delete[] y;
  delete[] o;
}

void manager_launch_kernel_test() {
  CudaManager cuda_manager;

  // Compile and get a kernel handle
  CudaCompiler cuda_compiler;
  char *ptx;
  size_t ptx_size;
  cuda_compiler.compile_to_ptx(KERNEL_PATH, &ptx, &ptx_size);

  CUmodule module;
  CUfunction kernel;
  CUDA_SAFE_CALL(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));
  CUDA_SAFE_CALL(cuModuleGetFunction(&kernel, module, KERNEL_NAME));

  // Free PTX
  delete[] ptx;

  // Setup input and output buffers
  size_t n = NUM_THREADS * NUM_BLOCKS;
  size_t buffer_size = n * sizeof(float);

  // Allocate and write buffers
  float *x, *y, *o;
  int xid = cuda_manager.memory_manager.allocate_buffer(buffer_size, (void **) &x);
  int yid = cuda_manager.memory_manager.allocate_buffer(buffer_size, (void **) &y);
  int oid = cuda_manager.memory_manager.allocate_buffer(buffer_size, (void **) &o);
  printf("Allocated buffer x id: %d\n", xid);
  printf("Allocated buffer y id: %d\n", yid);
  printf("Allocated buffer o id: %d\n", oid);

  float a = 2.5f;
  
  for (size_t i = 0; i < n; ++i) {
    x[i] = static_cast<float>(i);
    y[i] = static_cast<float>(i * 2);
  }

  ValueArg<float>  arg_a(a, sizeof(float), true);
  BufferArg arg_x(xid, buffer_size, true);
  BufferArg arg_y(yid, buffer_size, true);
  BufferArg arg_o(oid, buffer_size, false);
  ValueArg<float> arg_n(n, sizeof(float), true);

  std::vector<Arg *> args {&arg_a, &arg_x, &arg_y, &arg_o, &arg_n};

  cuda_manager.launch_kernel(kernel, args, NUM_BLOCKS, NUM_THREADS);

  for (size_t i = 0; i < 10; ++i) { // first 10 results only
    std::cout << a << " * " << x[i] << " + " << y[i] << " = " << o[i] << '\n';
  }

  // Free resources
  CUDA_SAFE_CALL(cuModuleUnload(module));
  delete[] x;
  delete[] y;
  delete[] o;
}

void manual_launch_kernel_test() {
  // Initialize cuda manager and compiler
  CudaManager cuda_manager;
  CudaCompiler cuda_compiler;

  // Compile the file to a PTX
  char *_ptx;
  cuda_compiler.compile_to_ptx(KERNEL_PATH, &_ptx);

  // Save the PTX file and read it again (for testing purposes)
  cuda_compiler.save_ptx_to_file(_ptx, PTX_PATH);
  delete[] _ptx;

  char *ptx = cuda_compiler.read_ptx_from_file(PTX_PATH);

  // Load PTX and get a kernel handle
  CUmodule module;
  CUfunction kernel;
  CUDA_SAFE_CALL(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));
  CUDA_SAFE_CALL(cuModuleGetFunction(&kernel, module, KERNEL_NAME));

  // Free PTX
  delete[] ptx;

  // Setup input and output buffers
  size_t n = NUM_THREADS * NUM_BLOCKS;
  size_t buffer_size = n * sizeof(float);
  float a = 2.5f;
  float *hX = new float[n], *hY = new float[n], *hOut = new float[n];
  
  for (size_t i = 0; i < n; ++i) {
    hX[i] = static_cast<float>(i);
    hY[i] = static_cast<float>(i * 2);
  }

  CUdeviceptr dX, dY, dOut;
  CUDA_SAFE_CALL(cuMemAlloc(&dX, buffer_size));
  CUDA_SAFE_CALL(cuMemAlloc(&dY, buffer_size));
  CUDA_SAFE_CALL(cuMemAlloc(&dOut, buffer_size));
  CUDA_SAFE_CALL(cuMemcpyHtoD(dX, hX, buffer_size));
  CUDA_SAFE_CALL(cuMemcpyHtoD(dY, hY, buffer_size));

  // Execute
  void *args[] = { &a, &dX, &dY, &dOut, &n };
  CUDA_SAFE_CALL(
    cuLaunchKernel(kernel, 
                   NUM_BLOCKS, 1, 1, // grid dim 
                   NUM_THREADS, 1, 1, // block dim
                   0, NULL, // shared mem, stream
                   args, 0) // args
  );
  // Syncronize
  CUDA_SAFE_CALL(cuCtxSynchronize());

  // Check results
  CUDA_SAFE_CALL(cuMemcpyDtoH(hOut, dOut, buffer_size));

  for (size_t i = 0; i < 10; ++i) { // first 10 results only
    std::cout << a << " * " << hX[i] << " + " << hY[i] << " = " << hOut[i] << '\n';
  }

  // Free resources
  CUDA_SAFE_CALL(cuMemFree(dX));
  CUDA_SAFE_CALL(cuMemFree(dY));
  CUDA_SAFE_CALL(cuMemFree(dOut));
  CUDA_SAFE_CALL(cuModuleUnload(module));
  delete[] hX;
  delete[] hY;
  delete[] hOut;
}

int main(void) {
  test_arg_parser();
  manager_launch_kernel_test();
  manual_launch_kernel_test();
}

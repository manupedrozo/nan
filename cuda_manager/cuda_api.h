#include "cuda_manager.h"

enum CudaApiExitCode {
  OK,
  ERROR
};

class CudaApi {
private:
  cuda_manager::CudaManager cuda_manager;
  cuda_manager::CudaMemoryManager memory_manager;

public:
  CudaApi();
  ~CudaApi();

  CudaApiExitCode allocate_memory(int mem_id, size_t size);
  CudaApiExitCode release_memory(int mem_id);
  CudaApiExitCode write_memory(int mem_id, void *data, size_t size);
  CudaApiExitCode read_memory(int mem_id, void *dest_buffer, size_t size);
  CudaApiExitCode launch_kernel(char *args, size_t size);
};
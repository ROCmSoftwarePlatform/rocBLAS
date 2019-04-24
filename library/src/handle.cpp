/* ************************************************************************
 * Copyright 2016 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "handle.h"
#include <cstdlib>
#include <numeric>

/*******************************************************************************
 * constructor
 ******************************************************************************/
_rocblas_handle::_rocblas_handle()
{
    // default device is active device
    THROW_IF_HIP_ERROR(hipGetDevice(&device));
    THROW_IF_HIP_ERROR(hipGetDeviceProperties(&device_properties, device));

    // rocblas by default take the system default stream 0 users cannot create

    // Device memory size
    //
    // If ROCBLAS_DEVICE_MEMORY_SIZE is set to > 0, then rocBLAS will allocate
    // the size specified, and will not manage device memory itself.
    //
    // If ROCBLAS_DEVICE_MEMORY_SIZE is unset, invalid or 0, then rocBLAS will
    // allocate a default initial size and manage device memory itself,
    // growing it as necessary.
    auto env = getenv("ROCBLAS_DEVICE_MEMORY_SIZE");
    device_memory_rocblas_managed =
        !env || sscanf(env, "%zu", &device_memory_size) != 1 || !device_memory_size;
    if(device_memory_rocblas_managed)
        device_memory_size = DEFAULT_DEVICE_MEMORY_SIZE;

    // Allocate device memory
    THROW_IF_HIP_ERROR(hipMalloc(&device_memory, device_memory_size));
}

/*******************************************************************************
 * destructor
 ******************************************************************************/
_rocblas_handle::~_rocblas_handle()
{
    // Deallocate device memory
    if(device_memory)
        hipFree(device_memory);
}

/*******************************************************************************
 * device memory allocator helper
 ******************************************************************************/
void* _rocblas_handle::device_memory_allocator(size_t size)
{
    if(device_memory_inuse)
    {
        std::cerr << "Device memory allocated twice without freeing" << std::endl;
        abort();
    }

    if(size > device_memory_size)
    {
        if(!device_memory_rocblas_managed)
            return nullptr;

        if(device_memory)
        {
            hipFree(device_memory);
            device_memory = nullptr;
        }
        device_memory_size = 0;

        if(hipMalloc(&device_memory, size) == hipSuccess)
            device_memory_size = size;
        else
            return nullptr;
    }

    device_memory_inuse = true;
    return device_memory;
}

/*******************************************************************************
 * allocation RAII object to return device memory for a rocblas kernel
 ******************************************************************************/
template <size_t N>
_rocblas_handle::_device_memory_alloc<N>::_device_memory_alloc(rocblas_handle handle,
                                                               std::initializer_list<size_t> sizes)
    : handle(handle)
{
    auto size    = sizes.begin();
    size_t total = 0;

#pragma unroll
    for(size_t i = 0; i < N; ++i)
        total += size[i];

    ptr[0] = handle->device_memory_allocator(total);

    if(ptr[0])
// If there are additional sizes, compute their offsets
#pragma unroll
        for(size_t i = 1; i < N; ++i)
            ptr[i]   = (char*)ptr[i - 1] + size0[i - 1];
}

// Modify a handle to query device memory size on the next rocblas call
extern "C" rocblas_handle rocblas_query_device_memory_size(rocblas_handle handle, size_t* size_p)
{
    handle->device_memory_size_query = size_p;
    return handle;
}

// Get the device memory size
extern "C" size_t rocblas_get_device_memory_size(rocblas_handle handle)
{
    return handle->device_memory_size;
}

// Set the device memory size
extern "C" rocblas_status rocblas_set_device_memory_size(rocblas_handle handle, size_t size)
{
    // Cannot change memory allocation when a device_memory_alloc
    // object is alive and using device memory.
    if(handle->device_memory_inuse)
        return rocblas_status_memory_error;

    // Free existing device memory, if any
    if(handle->device_memory)
    {
        hipFree(handle->device_memory);
        handle->device_memory = nullptr;
    }
    handle->device_memory_size = 0;

    // A zero size requests rocBLAS to take over management of device memory.
    // A nonzero size forces rocBLAS to use that as a fixed size, and not change it.
    if(!size)
    {
        handle->device_memory_rocblas_managed = true;
    }
    else
    {
        handle->device_memory_rocblas_managed = false;
        auto hipStatus                        = hipMalloc(&handle->device_memory, size);
        if(hipStatus != hipSuccess)
            return get_rocblas_status_for_hip_status(hipStatus);
        handle->device_memory_size = size;
    }
    return rocblas_status_success;
}

/*******************************************************************************
 * Static handle data
 ******************************************************************************/
rocblas_layer_mode _rocblas_handle::layer_mode = rocblas_layer_mode_none;
std::ofstream _rocblas_handle::log_trace_ofs;
std::ostream* _rocblas_handle::log_trace_os;
std::ofstream _rocblas_handle::log_bench_ofs;
std::ostream* _rocblas_handle::log_bench_os;
std::ofstream _rocblas_handle::log_profile_ofs;
std::ostream* _rocblas_handle::log_profile_os;
_rocblas_handle::init _rocblas_handle::handle_init;

/**
 *  @brief Logging function
 *
 *  @details
 *  open_log_stream Open stream log_os for logging.
 *                  If the environment variable with name environment_variable_name
 *                  is not set, then stream log_os to std::cerr.
 *                  Else open a file at the full logfile path contained in
 *                  the environment variable.
 *                  If opening the file suceeds, stream to the file
 *                  else stream to std::cerr.
 *
 *  @param[in]
 *  environment_variable_name   const char*
 *                              Name of environment variable that contains
 *                              the full logfile path.
 *
 *  @parm[out]
 *  log_os      std::ostream*&
 *              Output stream. Stream to std:cerr if environment_variable_name
 *              is not set, else set to stream to log_ofs
 *
 *  @parm[out]
 *  log_ofs     std::ofstream&
 *              Output file stream. If log_ofs->is_open()==true, then log_os
 *              will stream to log_ofs. Else it will stream to std::cerr.
 */

static void open_log_stream(const char* environment_variable_name,
                            std::ostream*& log_os,
                            std::ofstream& log_ofs)

{
    // By default, output to cerr
    log_os = &std::cerr;

    // if environment variable is set, open file at logfile_pathname contained in the
    // environment variable
    auto logfile_pathname = getenv(environment_variable_name);
    if(logfile_pathname)
    {
        log_ofs.open(logfile_pathname, std::ios_base::trunc);

        // if log_ofs is open, then stream to log_ofs, else log_os is already set to std::cerr
        if(log_ofs.is_open())
            log_os = &log_ofs;
    }
}

/*******************************************************************************
 * Static runtime initialization
 ******************************************************************************/
_rocblas_handle::init::init()
{
    // set layer_mode from value of environment variable ROCBLAS_LAYER
    auto str_layer_mode = getenv("ROCBLAS_LAYER");
    if(str_layer_mode)
    {
        layer_mode = static_cast<rocblas_layer_mode>(strtol(str_layer_mode, 0, 0));

        // open log_trace file
        if(layer_mode & rocblas_layer_mode_log_trace)
            open_log_stream("ROCBLAS_LOG_TRACE_PATH", log_trace_os, log_trace_ofs);

        // open log_bench file
        if(layer_mode & rocblas_layer_mode_log_bench)
            open_log_stream("ROCBLAS_LOG_BENCH_PATH", log_bench_os, log_bench_ofs);

        // open log_profile file
        if(layer_mode & rocblas_layer_mode_log_profile)
            open_log_stream("ROCBLAS_LOG_PROFILE_PATH", log_profile_os, log_profile_ofs);
    }
}

/*******************************************************************************
 * Static reinitialization (for testing only)
 ******************************************************************************/
namespace rocblas {
void reinit_logs()
{
    _rocblas_handle::log_trace_ofs.close();
    _rocblas_handle::log_bench_ofs.close();
    _rocblas_handle::log_profile_ofs.close();
    new(&_rocblas_handle::handle_init) _rocblas_handle::init;
}
} // namespace rocblas

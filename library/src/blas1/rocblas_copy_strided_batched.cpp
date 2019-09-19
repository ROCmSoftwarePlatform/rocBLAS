/* ************************************************************************
 * Copyright 2016-2019 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "handle.h"
#include "logging.h"
#include "rocblas.h"
#include "rocblas_copy.hpp"
#include "utility.h"

namespace
{
    template <typename>
    constexpr char rocblas_copy_strided_batched_name[] = "unknown";
    template <>
    constexpr char rocblas_copy_strided_batched_name<float>[] = "rocblas_scopy_strided_batched";
    template <>
    constexpr char rocblas_copy_strided_batched_name<double>[] = "rocblas_dcopy_strided_batched";
    template <>
    constexpr char rocblas_copy_strided_batched_name<rocblas_half>[]
        = "rocblas_hcopy_strided_batched";
    template <>
    constexpr char rocblas_copy_strided_batched_name<rocblas_float_complex>[]
        = "rocblas_ccopy_strided_batched";
    template <>
    constexpr char rocblas_copy_strided_batched_name<rocblas_double_complex>[]
        = "rocblas_zcopy_strided_batched";

    template <rocblas_int NB, typename T>
    rocblas_status rocblas_copy_strided_batched_impl(rocblas_handle handle,
                                                     rocblas_int    n,
                                                     const T*       x,
                                                     rocblas_int    incx,
                                                     rocblas_int    stridex,
                                                     T*             y,
                                                     rocblas_int    incy,
                                                     rocblas_int    stridey,
                                                     rocblas_stride    batch_count)
    {
        if(!handle)
            return rocblas_status_invalid_handle;

        auto layer_mode = handle->layer_mode;
        if(layer_mode & rocblas_layer_mode_log_trace)
            log_trace(handle,
                      rocblas_copy_strided_batched_name<T>,
                      n,
                      x,
                      incx,
                      stridex,
                      y,
                      incy,
                      stridey,
                      batch_count);

        if(layer_mode & rocblas_layer_mode_log_bench)
            log_bench(handle,
                      "./rocblas-bench -f copy_strided_batched -r",
                      rocblas_precision_string<T>,
                      "-n",
                      n,
                      "--incx",
                      incx,
                      "--stridex",
                      stridex,
                      "--incy",
                      incy,
                      "--stridey",
                      stridey,
                      "--batch_count",
                      batch_count);

        if(layer_mode & rocblas_layer_mode_log_profile)
            log_profile(handle,
                        rocblas_copy_strided_batched_name<T>,
                        "N",
                        n,
                        "incx",
                        incx,
                        "stridex",
                        stridex,
                        "incy",
                        incy,
                        "stridey",
                        stridey,
                        "batch_count",
                        batch_count);

        if(!x || !y)
            return rocblas_status_invalid_pointer;

        RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        if(!incx || !incy || batch_count < 0)
            return rocblas_status_invalid_size;

        return rocblas_copy_template<NB, T>(
            handle, n, x, 0, incx, stridex, y, 0, incy, stridey, batch_count);
    }

} // namespace

/* ============================================================================================ */

/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */

extern "C" {

rocblas_status rocblas_scopy_strided_batched(rocblas_handle handle,
                                             rocblas_int    n,
                                             const float*   x,
                                             rocblas_int    incx,
                                             rocblas_int    stridex,
                                             float*         y,
                                             rocblas_int    incy,
                                             rocblas_int    stridey,
                                             rocblas_stride    batch_count)
{
    constexpr rocblas_int NB = 256;
    return rocblas_copy_strided_batched_impl<NB>(
        handle, n, x, incx, stridex, y, incy, stridey, batch_count);
}

rocblas_status rocblas_dcopy_strided_batched(rocblas_handle handle,
                                             rocblas_int    n,
                                             const double*  x,
                                             rocblas_int    incx,
                                             rocblas_int    stridex,
                                             double*        y,
                                             rocblas_int    incy,
                                             rocblas_int    stridey,
                                             rocblas_stride    batch_count)
{
    constexpr rocblas_int NB = 256;
    return rocblas_copy_strided_batched_impl<NB>(
        handle, n, x, incx, stridex, y, incy, stridey, batch_count);
}

rocblas_status rocblas_hcopy_strided_batched(rocblas_handle      handle,
                                             rocblas_int         n,
                                             const rocblas_half* x,
                                             rocblas_int         incx,
                                             rocblas_int         stridex,
                                             rocblas_half*       y,
                                             rocblas_int         incy,
                                             rocblas_int         stridey,
                                             rocblas_stride         batch_count)
{
    constexpr rocblas_int NB = 256;
    return rocblas_copy_strided_batched_impl<NB>(
        handle, n, x, incx, stridex, y, incy, stridey, batch_count);
}

rocblas_status rocblas_ccopy_strided_batched(rocblas_handle               handle,
                                             rocblas_int                  n,
                                             const rocblas_float_complex* x,
                                             rocblas_int                  incx,
                                             rocblas_int                  stridex,
                                             rocblas_float_complex*       y,
                                             rocblas_int                  incy,
                                             rocblas_int                  stridey,
                                             rocblas_stride                  batch_count)
{
    constexpr rocblas_int NB = 256;
    return rocblas_copy_strided_batched_impl<NB>(
        handle, n, x, incx, stridex, y, incy, stridey, batch_count);
}

rocblas_status rocblas_zcopy_strided_batched(rocblas_handle                handle,
                                             rocblas_int                   n,
                                             const rocblas_double_complex* x,
                                             rocblas_int                   incx,
                                             rocblas_int                   stridex,
                                             rocblas_double_complex*       y,
                                             rocblas_int                   incy,
                                             rocblas_int                   stridey,
                                             rocblas_stride                   batch_count)
{
    constexpr rocblas_int NB = 256;
    return rocblas_copy_strided_batched_impl<NB>(
        handle, n, x, incx, stridex, y, incy, stridey, batch_count);
}

} // extern "C"

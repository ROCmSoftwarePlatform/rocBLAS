/* ************************************************************************
 * Copyright 2016-2020 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "rocblas_symm.hpp"
#include "logging.h"
#include "utility.h"

namespace
{
    template <typename>
    constexpr char rocblas_symm_name[] = "unknown";
    template <>
    constexpr char rocblas_symm_name<float>[] = "rocblas_ssymm";
    template <>
    constexpr char rocblas_symm_name<double>[] = "rocblas_dsymm";
    template <>
    constexpr char rocblas_symm_name<rocblas_float_complex>[] = "rocblas_csymm";
    template <>
    constexpr char rocblas_symm_name<rocblas_double_complex>[] = "rocblas_zsymm";

    template <typename T>
    rocblas_status rocblas_symm_impl(rocblas_handle handle,
                                     rocblas_side   side,
                                     rocblas_fill   uplo,
                                     rocblas_int    m,
                                     rocblas_int    n,
                                     const T*       alpha,
                                     const T*       A,
                                     rocblas_int    lda,
                                     const T*       B,
                                     rocblas_int    ldb,
                                     const T*       beta,
                                     T*             C,
                                     rocblas_int    ldc)
    {
        if(!handle)
            return rocblas_status_invalid_handle;

        RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        auto layer_mode = handle->layer_mode;
        if(layer_mode
           & (rocblas_layer_mode_log_trace | rocblas_layer_mode_log_bench
              | rocblas_layer_mode_log_profile))
        {
            auto uplo_letter = rocblas_fill_letter(uplo);
            auto side_letter = rocblas_side_letter(side);

            if(handle->pointer_mode == rocblas_pointer_mode_host)
            {
                if(layer_mode & rocblas_layer_mode_log_trace)
                    log_trace(handle,
                              rocblas_symm_name<T>,
                              side,
                              uplo,
                              m,
                              n,
                              log_trace_scalar_value(alpha),
                              A,
                              lda,
                              B,
                              ldb,
                              log_trace_scalar_value(beta),
                              C,
                              ldc);

                if(layer_mode & rocblas_layer_mode_log_bench)
                    log_bench(handle,
                              "./rocblas-bench -f symm -r",
                              rocblas_precision_string<T>,
                              "--side",
                              side_letter,
                              "--uplo",
                              uplo_letter,
                              "-m",
                              m,
                              "-n",
                              n,
                              LOG_BENCH_SCALAR_VALUE(alpha),
                              "--lda",
                              lda,
                              "--ldb",
                              ldb,
                              LOG_BENCH_SCALAR_VALUE(beta),
                              "--ldc",
                              ldc);
            }
            else
            {
                if(layer_mode & rocblas_layer_mode_log_trace)
                    log_trace(handle,
                              rocblas_symm_name<T>,
                              side,
                              uplo,
                              m,
                              n,
                              log_trace_scalar_value(alpha),
                              A,
                              lda,
                              B,
                              ldb,
                              log_trace_scalar_value(beta),
                              C,
                              ldc);
            }

            if(layer_mode & rocblas_layer_mode_log_profile)
                log_profile(handle,
                            rocblas_symm_name<T>,
                            "side",
                            side_letter,
                            "uplo",
                            uplo_letter,
                            "M",
                            m,
                            "N",
                            n,
                            "lda",
                            lda,
                            "ldb",
                            ldb,
                            "ldc",
                            ldc);
        }

        static constexpr rocblas_int    offset_C = 0, offset_A = 0, offset_B = 0, batch_count = 1;
        static constexpr rocblas_stride stride_C = 0, stride_A = 0, stride_B = 0;

        rocblas_status arg_status = rocblas_symm_arg_check(handle,
                                                           side,
                                                           uplo,
                                                           m,
                                                           n,
                                                           alpha,
                                                           A,
                                                           offset_A,
                                                           lda,
                                                           stride_A,
                                                           B,
                                                           offset_B,
                                                           ldb,
                                                           stride_B,
                                                           beta,
                                                           C,
                                                           offset_C,
                                                           ldc,
                                                           stride_C,
                                                           batch_count);
        if(arg_status != rocblas_status_continue)
            return arg_status;

        return rocblas_symm_template<false>(handle,
                                            side,
                                            uplo,
                                            m,
                                            n,
                                            alpha,
                                            A,
                                            offset_A,
                                            lda,
                                            stride_A,
                                            B,
                                            offset_B,
                                            ldb,
                                            stride_B,
                                            beta,
                                            C,
                                            offset_C,
                                            ldc,
                                            stride_C,
                                            batch_count);
    }

}
/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */

extern "C" {

#ifdef IMPL
#error IMPL ALREADY DEFINED
#endif

#define IMPL(routine_name_, T_)                                                                  \
    rocblas_status routine_name_(rocblas_handle handle,                                          \
                                 rocblas_side   side,                                            \
                                 rocblas_fill   uplo,                                            \
                                 rocblas_int    m,                                               \
                                 rocblas_int    n,                                               \
                                 const T_*      alpha,                                           \
                                 const T_*      A,                                               \
                                 rocblas_int    lda,                                             \
                                 const T_*      B,                                               \
                                 rocblas_int    ldb,                                             \
                                 const T_*      beta,                                            \
                                 T_*            C,                                               \
                                 rocblas_int    ldc)                                             \
    try                                                                                          \
    {                                                                                            \
        return rocblas_symm_impl(handle, side, uplo, m, n, alpha, A, lda, B, ldb, beta, C, ldc); \
    }                                                                                            \
    catch(...)                                                                                   \
    {                                                                                            \
        return exception_to_rocblas_status();                                                    \
    }

IMPL(rocblas_ssymm, float);
IMPL(rocblas_dsymm, double);
IMPL(rocblas_csymm, rocblas_float_complex);
IMPL(rocblas_zsymm, rocblas_double_complex);

#undef IMPL

} // extern "C"

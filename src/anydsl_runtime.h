#ifndef ANYDSL_RUNTIME_H
#define ANYDSL_RUNTIME_H

#include <stdint.h>
#include <stdlib.h>

#include "anydsl_runtime_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ANYDSL_DEVICE(p, d) ((p) | ((d) << 4))

enum {
    ANYDSL_HOST = 0,
    ANYDSL_CUDA = 1,
    ANYDSL_OPENCL = 2,
    ANYDSL_HSA = 3
};

AnyDSL_runtime_API void anydsl_info(void);

AnyDSL_runtime_API const char* anydsl_device_name(int32_t);
AnyDSL_runtime_API bool anydsl_device_check_feature_support(int32_t, const char*);

AnyDSL_runtime_API void* anydsl_alloc(int32_t, int64_t);
AnyDSL_runtime_API void* anydsl_alloc_host(int32_t, int64_t);
AnyDSL_runtime_API void* anydsl_alloc_unified(int32_t, int64_t);
AnyDSL_runtime_API void* anydsl_get_device_ptr(int32_t, void*);
AnyDSL_runtime_API void  anydsl_release(int32_t, void*);
AnyDSL_runtime_API void  anydsl_release_host(int32_t, void*);

AnyDSL_runtime_API void anydsl_copy(int32_t, const void*, int64_t, int32_t, void*, int64_t, int64_t);

AnyDSL_runtime_API void anydsl_launch_kernel(
    int32_t, const char*, const char*,
    const uint32_t*, const uint32_t*,
    void**, const uint32_t*, const uint32_t*, const uint32_t*, const uint8_t*,
    uint32_t);
AnyDSL_runtime_API void anydsl_synchronize(int32_t);

AnyDSL_runtime_API void anydsl_random_seed(uint32_t);
AnyDSL_runtime_API float    anydsl_random_val_f32();
AnyDSL_runtime_API uint64_t anydsl_random_val_u64();

AnyDSL_runtime_API uint64_t anydsl_get_micro_time();
AnyDSL_runtime_API uint64_t anydsl_get_nano_time();
AnyDSL_runtime_API uint64_t anydsl_get_kernel_time();

AnyDSL_runtime_API int32_t anydsl_isinff(float);
AnyDSL_runtime_API int32_t anydsl_isnanf(float);
AnyDSL_runtime_API int32_t anydsl_isfinitef(float);
AnyDSL_runtime_API int32_t anydsl_isinf(double);
AnyDSL_runtime_API int32_t anydsl_isnan(double);
AnyDSL_runtime_API int32_t anydsl_isfinite(double);

AnyDSL_runtime_API void anydsl_print_i16(int16_t);
AnyDSL_runtime_API void anydsl_print_i32(int32_t);
AnyDSL_runtime_API void anydsl_print_i64(int64_t);
AnyDSL_runtime_API void anydsl_print_u16(uint16_t);
AnyDSL_runtime_API void anydsl_print_u32(uint32_t);
AnyDSL_runtime_API void anydsl_print_u64(uint64_t);
AnyDSL_runtime_API void anydsl_print_f32(float);
AnyDSL_runtime_API void anydsl_print_f64(double);
AnyDSL_runtime_API void anydsl_print_char(char);
AnyDSL_runtime_API void anydsl_print_string(char*);
AnyDSL_runtime_API void anydsl_print_flush();

AnyDSL_runtime_API void* anydsl_aligned_malloc(size_t, size_t);
AnyDSL_runtime_API void anydsl_aligned_free(void*);

AnyDSL_runtime_API void anydsl_parallel_for(int32_t, int32_t, int32_t, void*, void*);
AnyDSL_runtime_API int32_t anydsl_spawn_thread(void*, void*);
AnyDSL_runtime_API void anydsl_sync_thread(int32_t);

struct AnyDSL_runtime_API Closure {
    void (*fn)(uint64_t);
    uint64_t payload;
};

AnyDSL_runtime_API int32_t anydsl_create_graph();
AnyDSL_runtime_API int32_t anydsl_create_task(int32_t, Closure);
AnyDSL_runtime_API void    anydsl_create_edge(int32_t, int32_t);
AnyDSL_runtime_API void    anydsl_execute_graph(int32_t, int32_t);

typedef uint64_t anydsl_event_t;
/// Create event for device. Will return id of event
AnyDSL_runtime_API anydsl_event_t anydsl_create_event(int32_t);
/// Destroy event
AnyDSL_runtime_API void anydsl_destroy_event(int32_t, anydsl_event_t);
/// Record the event for the device
AnyDSL_runtime_API void anydsl_record_event(int32_t, anydsl_event_t);
/// Check if event has completed. True if the event is completed, false otherwise
AnyDSL_runtime_API bool anydsl_check_event(int32_t, anydsl_event_t);
/// Query time between two events in micro seconds. Both events have to be completed, else UINT64_MAX is returned
AnyDSL_runtime_API uint64_t anydsl_query_us_event(int32_t, anydsl_event_t, anydsl_event_t);
/// Wait for the event to complete
AnyDSL_runtime_API void anydsl_sync_event(int32_t, anydsl_event_t);

#ifdef __cplusplus
}
#include "anydsl_runtime.hpp"
#endif

#endif

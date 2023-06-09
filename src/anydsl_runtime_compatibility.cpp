#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>

#include "anydsl_runtime.h"
#include "anydsl_runtime_internal_config.h"
#include "runtime.h"

#ifdef AnyDSL_runtime_HAS_TBB_SUPPORT
#define NOMINMAX
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>
#else
#include <thread>
#endif

#define AnyDSL_runtime_std_API AnyDSL_runtime_API

// This file contains stuff not put anywhere else.
// NOTE: Random functions are removed. Ain't the business of a runtime to define these.

extern "C" {
AnyDSL_runtime_std_API uint64_t anydsl_std_get_micro_time()
{
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}

AnyDSL_runtime_std_API uint64_t anydsl_std_get_nano_time()
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

AnyDSL_runtime_std_API int32_t anydsl_std_isinff(float x) { return std::isinf(x); }
AnyDSL_runtime_std_API int32_t anydsl_std_isnanf(float x) { return std::isnan(x); }
AnyDSL_runtime_std_API int32_t anydsl_std_isfinitef(float x) { return std::isfinite(x); }
AnyDSL_runtime_std_API int32_t anydsl_std_isinf(double x) { return std::isinf(x); }
AnyDSL_runtime_std_API int32_t anydsl_std_isnan(double x) { return std::isnan(x); }
AnyDSL_runtime_std_API int32_t anydsl_std_isfinite(double x) { return std::isfinite(x); }

AnyDSL_runtime_std_API void anydsl_std_print_i16(int16_t s) { std::cout << s; }
AnyDSL_runtime_std_API void anydsl_std_print_i32(int32_t i) { std::cout << i; }
AnyDSL_runtime_std_API void anydsl_std_print_i64(int64_t l) { std::cout << l; }
AnyDSL_runtime_std_API void anydsl_std_print_u16(uint16_t s) { std::cout << s; }
AnyDSL_runtime_std_API void anydsl_std_print_u32(uint32_t i) { std::cout << i; }
AnyDSL_runtime_std_API void anydsl_std_print_u64(uint64_t l) { std::cout << l; }
AnyDSL_runtime_std_API void anydsl_std_print_f32(float f) { std::cout << f; }
AnyDSL_runtime_std_API void anydsl_std_print_f64(double d) { std::cout << d; }
AnyDSL_runtime_std_API void anydsl_std_print_char(char c) { std::cout << c; }
AnyDSL_runtime_std_API void anydsl_std_print_string(char* s) { std::cout << s; }
AnyDSL_runtime_std_API void anydsl_std_print_flush() { std::cout << std::flush; }

AnyDSL_runtime_std_API void* anydsl_std_aligned_malloc(size_t size, size_t align)
{
    return AnyDSLInternal::Runtime::instance().aligned_malloc(size, align);
}

AnyDSL_runtime_std_API void anydsl_std_aligned_free(void* ptr)
{
    return AnyDSLInternal::Runtime::instance().aligned_free(ptr);
}

#ifndef AnyDSL_runtime_HAS_TBB_SUPPORT // C++11 threads version
static std::unordered_map<int32_t, std::thread> thread_pool;
static std::vector<int32_t> free_ids;
static std::mutex thread_lock;

AnyDSL_runtime_std_API void anydsl_std_parallel_for(int32_t num_threads, int32_t lower, int32_t upper, void* args, void* fun)
{
    // Get number of available hardware threads
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        // hardware_concurrency is implementation defined, may return 0
        num_threads = (num_threads == 0) ? 1 : num_threads;
    }

    void (*fun_ptr)(void*, int32_t, int32_t) = reinterpret_cast<void (*)(void*, int32_t, int32_t)>(fun);
    const int32_t linear                     = (upper - lower) / num_threads;

    // Create a pool of threads to execute the task
    std::vector<std::thread> pool(num_threads);

    for (int i = 0, a = lower, b = lower + linear; i < num_threads - 1; a = b, b += linear, i++) {
        pool[i] = std::thread([=]() {
            fun_ptr(args, a, b);
        });
    }

    pool[num_threads - 1] = std::thread([=]() {
        fun_ptr(args, lower + (num_threads - 1) * linear, upper);
    });

    // Wait for all the threads to finish
    for (int i = 0; i < num_threads; i++)
        pool[i].join();
}

AnyDSL_runtime_std_API int32_t anydsl_std_spawn_thread(void* args, void* fun)
{
    std::lock_guard<std::mutex> lock(thread_lock);

    int32_t (*fun_ptr)(void*) = reinterpret_cast<int32_t (*)(void*)>(fun);

    int32_t id;
    if (free_ids.size()) {
        id = free_ids.back();
        free_ids.pop_back();
    } else {
        id = static_cast<int32_t>(thread_pool.size());
    }

    auto spawned = std::make_pair(id, std::thread([=]() { fun_ptr(args); }));
    thread_pool.emplace(std::move(spawned));
    return id;
}

AnyDSL_runtime_std_API void anydsl_std_sync_thread(int32_t id)
{
    auto thread = thread_pool.end();
    {
        std::lock_guard<std::mutex> lock(thread_lock);
        thread = thread_pool.find(id);
    }
    if (thread != thread_pool.end()) {
        thread->second.join();
        {
            std::lock_guard<std::mutex> lock(thread_lock);
            free_ids.push_back(thread->first);
            thread_pool.erase(thread);
        }
    } else {
        assert(0 && "Trying to synchronize on invalid thread id");
    }
}
#else // TBB version
AnyDSL_runtime_std_API void anydsl_std_parallel_for(int32_t num_threads, int32_t lower, int32_t upper, void* args, void* fun)
{
    tbb::task_arena limited((num_threads == 0) ? tbb::task_arena::automatic : num_threads);
    tbb::task_group tg;

    void (*fun_ptr)(void*, int32_t, int32_t) = reinterpret_cast<void (*)(void*, int32_t, int32_t)>(fun);

    limited.execute([&] {
        tg.run([&] {
            tbb::parallel_for(tbb::blocked_range<int32_t>(lower, upper),
                              [=](const tbb::blocked_range<int32_t>& range) {
                                  fun_ptr(args, range.begin(), range.end());
                              });
        });
    });

    limited.execute([&] { tg.wait(); });
}

typedef tbb::concurrent_unordered_map<int32_t, tbb::task_group, std::hash<int32_t>> task_group_map;
typedef std::pair<task_group_map::iterator, bool> task_group_node_ref;
static task_group_map task_pool;
static tbb::concurrent_queue<int32_t> free_ids;
static std::mutex thread_lock;

AnyDSL_runtime_std_API int32_t anydsl_std_spawn_thread(void* args, void* fun)
{
    std::lock_guard<std::mutex> lock(thread_lock);
    int32_t id = -1;
    if (!free_ids.try_pop(id)) {
        id = int32_t(task_pool.size());
    }

    int32_t (*fun_ptr)(void*) = reinterpret_cast<int32_t (*)(void*)>(fun);

    assert(id >= 0);

    task_group_node_ref p = task_pool.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple());
    tbb::task_group& tg   = p.first->second;

    tg.run([=] { fun_ptr(args); });

    return id;
}

AnyDSL_runtime_std_API void anydsl_std_sync_thread(int32_t id)
{
    auto task = task_pool.end();
    {
        std::lock_guard<std::mutex> lock(thread_lock);
        task = task_pool.find(id);
    }
    if (task != task_pool.end()) {
        task->second.wait();
        {
            std::lock_guard<std::mutex> lock(thread_lock);
            free_ids.push(task->first);
        }
    } else {
        assert(0 && "Trying to synchronize on invalid task id");
    }
}
#endif

} // extern "C"

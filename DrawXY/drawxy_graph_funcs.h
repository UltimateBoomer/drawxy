#pragma once

#include "drawxy_common.h"
#include "drawxy_draw_funcs.h"
#include "drawxy_structs.h"

#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

enum class bench_type
{
    MULTISAMPLE,
    FIXED_TIME
};

template <typename T> const std::vector<float> graph_single(const T(*func)(T, T), const int size, const float scale_x, const float scale_y,
    const float offset_x, const float offset_y);

template <> const std::vector<float> graph_single<float>(const float(*func)(float, float), const int size, const float scale_x, const float scale_y,
    const float offset_x, const float offset_y)
{
    const int size2 = size * size;

    std::vector<float> result(size2);

    for (int i = 0; i < size2; i++)
    {
        // Calculate coordinates from index
        const int cx = i % size;
        const int cy = i / size;

        const float x = (float)cx / size * scale_x + offset_x;
        const float y = (float)cy / size * scale_y + offset_y;

        result[i] = func(x, y);
    }

    return result;
}

template <> const std::vector<float> graph_single<Vec8f>(const Vec8f(*func)(Vec8f, Vec8f), const int size, const float scale_x, const float scale_y,
    const float offset_x, const float offset_y)
{
    static constexpr int group_size = 8;

    const int size2 = size * size;

    const Vec8f size_v(size);
    const Vec8f scale_x_v(scale_x);
    const Vec8f scale_y_v(scale_y);
    const Vec8f offset_x_v(offset_x);
    const Vec8f offset_y_v(offset_y);

    const Vec8f scale_x_o_size_v = scale_x_v / size_v;
    const Vec8f scale_y_o_size_v = scale_y_v / size_v;

    std::vector<float> result(size2);

    for (int i = 0; i < size2; i += group_size)
    {
        Vec8f i_v(i);
        i_v += ci_v8;

        // x vector
        // cx = i % size = i - roundto0(i / size) * size
        // x = cx / size * scale_x + offset_x
        Vec8f x_v = i_v;

        x_v = x_v / size_v;
        x_v = truncate(x_v);
        x_v = nmul_add(x_v, size_v, i_v);

        x_v = mul_add(x_v, scale_x_o_size_v, offset_x_v);

        // y vector
        // cy = i / size
        // y = cy / size * scale_y + offset_y
        Vec8f y_v = i_v;

        y_v = y_v / size_v;

        y_v = mul_add(y_v, scale_y_o_size_v, offset_y_v);

        // Result
        Vec8f r_v = func(x_v, y_v);

        r_v.store(&result[i]);
    }

    return result;
}

template <graph_shape shape> const float calc_avg_s(const int samples, const float scale_x, const float scale_y,
    const float offset_x, const float offset_y)
{
    const int samples2 = samples * samples;

    float avg = 0.0f;

    for (int i = 0; i < samples2; i++)
    {
        // Calculate coordinates from index
        const int cx = i % samples;
        const int cy = i / samples;

        const float x = (float)cx / samples * scale_x + offset_x;
        const float y = (float)cy / samples * scale_y + offset_y;

        avg += draw_func<float, shape>(x, y);
    }

    return avg / samples2;
}

template <graph_shape shape> const float calc_avg_v8(const int samples, const float scale_x, const float scale_y,
    const float offset_x, const float offset_y)
{
    static constexpr int group_size = 8;

    const int samples2 = samples * samples;

    const Vec8f size_v(samples);
    const Vec8f scale_x_v(scale_x);
    const Vec8f scale_y_v(scale_y);
    const Vec8f offset_x_v(offset_x);
    const Vec8f offset_y_v(offset_y);

    const Vec8f scale_x_o_size_v = scale_x_v / size_v;
    const Vec8f scale_y_o_size_v = scale_y_v / size_v;

    float avg = 0.0f;
    Vec8f sum_v(0);

#pragma clang loop unroll(disable)
    for (int i = 0; i < samples2; i += group_size)
    {
        Vec8f i_v(i);
        i_v += ci_v8;

        // x vector
        // cx = i % size = i - roundto0(i / size) * size
        // x = cx / size * scale_x + offset_x
        Vec8f x_v = i_v;

        // y vector
        // cy = i / size
        // y = cy / size * scale_y + offset_y
        Vec8f y_v = i_v;

        x_v /= size_v;
        y_v /= size_v;

        x_v = truncate(x_v);
        x_v = nmul_add(x_v, size_v, i_v);

        x_v = mul_add(x_v, scale_x_o_size_v, offset_x_v);
        
        y_v = mul_add(y_v, scale_y_o_size_v, offset_y_v);

        // Result
        Vec8f r_v = draw_func<Vec8f, shape>(x_v, y_v);
        sum_v += r_v;
    }

    avg = horizontal_add(sum_v) / samples2;
    return avg;
}

template <vectorization_level vl, graph_shape shape> constexpr float calc_avg(const int samples, const float scale_x, const float scale_y,
    const float offset_x, const float offset_y)
{
    if constexpr (vl == vectorization_level::NONE)
    {
        return calc_avg_s<shape>(samples, scale_x, scale_y, offset_x, offset_y);
    }
    else if constexpr (vl == vectorization_level::AVX2)
    {
        return calc_avg_v8<shape>(samples, scale_x, scale_y, offset_x, offset_y);
    }
}

template <typename T, graph_shape shape> const multisample_run_result graph_multisample_direct( int size, int samples,
    float scale_x, float scale_y, float offset_x, float offset_y)
{
    const float size2 = size * size;

    multisample_graph_result result;
    result.graph = std::vector<float>(size2);

    long long best_single_time = INT64_MAX;
    long long sum_single_time = 0;

    float scale_x_p = scale_x / size;
    float scale_y_p = scale_y / size;

    auto begin_time = std::chrono::steady_clock::now();

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            auto begin_time_inner = std::chrono::steady_clock::now();

            float offset_x_p = offset_x + x * scale_x_p;
            float offset_y_p = offset_y + y * scale_y_p;

            result.graph[x + y * size] = calc_avg<T, shape>(samples, scale_x_p, scale_y_p, offset_x_p, offset_y_p);

            auto end_time_inner = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_inner - begin_time_inner).count();

            sum_single_time += duration;
            if (best_single_time > duration)
                best_single_time = duration;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - begin_time).count();

    result.sum_single_time = sum_single_time;
    result.best_single_time = best_single_time;
    result.total_time = duration;

    return result;
}

template <vectorization_level vl, graph_shape shape> const multisample_run_result graph_multisample_mt(int size, int samples,
    float scale_x, float scale_y, float offset_x, float offset_y, int threads, std::vector<float>& graph)
{
    const float size2 = size * size;
    
    multisample_run_result result;
    graph = std::vector<float>(size2);

    std::atomic<long long> best_single_time = INT64_MAX;
    std::atomic<long long> sum_single_time = 0;

    float scale_x_p = scale_x / size;
    float scale_y_p = scale_y / size;

    std::vector<std::thread> thread_group(threads);
    std::atomic<int> n = 0;

    bool ready = false;

    std::mutex m;
    std::condition_variable cv;

    static const auto run_calc = [&]
    {
        std::mutex m_run;
        std::unique_lock<std::mutex> lk_ready(m_run);
        cv.wait(lk_ready, [&] { return ready; });

        for (int i = n++; i < size2; i = n++)
        {
            int x = i % size;
            int y = i / size;

            float offset_x_p = offset_x + x * scale_x_p;
            float offset_y_p = offset_y + y * scale_y_p;


            auto begin_time = std::chrono::steady_clock::now();

            graph[i] = calc_avg<vl, shape>(samples, scale_x_p, scale_y_p, offset_x_p, offset_y_p);

            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - begin_time).count();

            sum_single_time += duration;
            if (best_single_time > duration)
                best_single_time = duration;
        }

        cv.notify_all();
    };

    for (int i = 0; i < threads; i++)
    {
        thread_group[i] = std::thread(run_calc);
    }

    auto begin_time = std::chrono::steady_clock::now();

    ready = true;
    cv.notify_all();

    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return n >= size2; });
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - begin_time).count();

    result.sum_single_time = sum_single_time;
    result.best_single_time = best_single_time;
    result.total_time = duration;

    for (int i = 0; i < threads; i++)
    {
        thread_group[i].join();
    }

    return result;
}

template <vectorization_level vl, graph_shape shape> const auto graph_fixedtime_mt(int size, int samples,
    float scale_x, float scale_y, float offset_x, float offset_y, int threads, long long time)
{
    const int size2 = size * size;
    
    fixedtime_run_result result;

    std::vector<float> graph(size2);

    float scale_x_p = scale_x / size;
    float scale_y_p = scale_y / size;

    std::vector<std::thread> thread_group(threads);
    std::atomic<int> n = 0;

    bool ready = false;

    std::mutex m;
    std::condition_variable cv;

    static const auto run_calc = [&]
    {
        std::mutex m_run;
        std::unique_lock<std::mutex> lk_ready(m_run);
        cv.wait(lk_ready, [&] { return ready; });

        for (int i = n++; ready; i = n++)
        {
            int x = i % size;
            int y = i / size % size2;

            float offset_x_p = offset_x + x * scale_x_p;
            float offset_y_p = offset_y + y * scale_y_p;

            graph[i % size2] = calc_avg<vl, shape>(samples, scale_x_p, scale_y_p, offset_x_p, offset_y_p);
        }
    };

    for (int i = 0; i < threads; i++)
    {
        thread_group[i] = std::thread(run_calc);
    }

    auto begin_time = std::chrono::steady_clock::now();

    ready = true;
    cv.notify_all();

    std::this_thread::sleep_until(begin_time + std::chrono::milliseconds(time));
    ready = false;

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - begin_time).count();

    result.count = n;
    result.time = duration;

    for (int i = 0; i < threads; i++)
    {
        thread_group[i].join();
    }

    return result;
}

#pragma once

#include <iostream>
#include <numeric>

#include "drawxy_common.h"
#include "drawxy_graph_funcs.h"
#include "drawxy_structs.h"

constexpr float scale_x = 4.0;
constexpr float scale_y = 4.0;
constexpr float offset_x = -2.0;
constexpr float offset_y = -2.0;
constexpr float threshold = 0.5;

template <graph_shape shape> multisample_run_result run_multisample_single(const run_params params)
{
    const auto size2 = params.size * params.size;

    multisample_run_result result;
    std::vector<float> graph(size2);

    if (params.vec_level == vectorization_level::NONE)
    {
        result = graph_multisample_mt<vectorization_level::NONE, shape>(params.size, params.samples, scale_x, scale_y, offset_x, offset_y, params.threads, graph);
    }
    else
    {
        result = graph_multisample_mt<vectorization_level::AVX2, shape>(params.size, params.samples, scale_x, scale_y, offset_x, offset_y, params.threads, graph);
    }

    auto avg_value = std::accumulate(graph.begin(), graph.end(), 0.f) / size2;



    auto est_mt_time = result.sum_single_time / params.threads;
    auto overhead = (float) (result.total_time - est_mt_time) / est_mt_time * 100;
    auto perf = params.total_calculations() * 1e9 / result.total_time;
    auto score = result.score();

    std::cout << "Score:                " << score << std::endl;
    std::cout << "Total time:           " << result.total_time / 1e6 << " ms" << std::endl;
    std::cout << "Avg time/unit:        " << result.sum_single_time / size2 / 1e6 << " ms" << std::endl;
    std::cout << "Best time/unit:       " << result.best_single_time / 1e6 << " ms" << std::endl;
    std::cout << "ST total time:        " << result.sum_single_time / 1e6 << " ms" << std::endl;
    std::cout << "Est MT total time:    " << est_mt_time / 1e6 << " ms" << std::endl;
    std::cout << "Overhead:             " << overhead << "%" << std::endl;
    std::cout << "Avg value:            " << avg_value << std::endl;
    std::cout << "Performance:          " << perf << " calc/s" << std::endl;
    std::cout << std::endl;

#ifdef PRINT_RESULT
    for (int y = size - 1; y >= 0; y--)
    {
        for (int x = 0; x < size; x++)
        {
            std::cout << (result.graph[x + y * size] > threshold ? "X " : ". ");
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
#endif
    
    return result;
}

template <graph_shape shape> long long run_multisample_loop(const run_params params, const int count)
{
    std::cout << "Multisample Benchmark" << std::endl;
    std::cout << "Shape:                " << graph_shape_to_string(shape) << std::endl;
    std::cout << "Vectorization level:  " << vec_level_to_string(params.vec_level) << std::endl;
    std::cout << "Samples Per Unit:     " << params.samples << std::endl;
    std::cout << "Display Size:         " << params.size << std::endl;
    std::cout << "Threads:              " << params.threads << std::endl;
    std::cout << "Total Calculations:   " << params.total_calculations() << std::endl;
    std::cout << "Runs:                 " << count << std::endl;
    std::cout << std::endl;
    
    long long hi_score = 0;

    long long sum_time = 0;
    long long best_time = INT64_MAX;

    for (int i = 0; i < count; i++)
    {
        auto result = run_multisample_single<shape>(params);
        auto score = result.score();

        hi_score = score > hi_score ? score : hi_score;

        sum_time += result.total_time;
        best_time = result.total_time < best_time ? result.total_time : best_time;
    }

    long long avg_time = sum_time / count;

    std::cout << "Loop finished" << std::endl;
    std::cout << "Score:                " << hi_score << std::endl;
    std::cout << "Avg time:             " << avg_time / 1e6 << " ms" << std::endl;
    std::cout << "Best time:            " << best_time / 1e6 << " ms" << std::endl;
    std::cout << std::endl;

    return hi_score;
};

template <graph_shape shape> fixedtime_run_result run_fixedtime_single(const run_params params, const long long time)
{
    const auto size2 = params.size * params.size;

    fixedtime_run_result result;

    if (params.vec_level == vectorization_level::NONE)
    {
        result = graph_fixedtime_mt<vectorization_level::NONE, shape>(params.size, params.samples, scale_x, scale_y, offset_x, offset_y, params.threads, time);
    }
    else
    {
        result = graph_fixedtime_mt<vectorization_level::AVX2, shape>(params.size, params.samples, scale_x, scale_y, offset_x, offset_y, params.threads, time);
    }
    
    auto score = result.score();

    std::cout << "Score:                " << score << std::endl;
    std::cout << "Units prcoessed:      " << result.count << std::endl;
    std::cout << "Precise time:         " << result.time / 1e6 << " ms" << std::endl;
    std::cout << std::endl;

    return result;
}

template <graph_shape shape> long long run_fixedtime_loop(const run_params params, const long long time, const int count)
{
    std::cout << "Fixed Time Benchmark" << std::endl;
    std::cout << "Shape:                " << graph_shape_to_string(shape) << std::endl;
    std::cout << "Vectorization level:  " << vec_level_to_string(params.vec_level) << std::endl;
    std::cout << "Threads:              " << params.threads << std::endl;
    std::cout << "Time:                 " << time << " ms" << std::endl;
    std::cout << std::endl;
    
    long long hi_score = 0;

    for (int i = 0; i < count; i++)
    {
        std::cout << "Run " << i + 1 << "/" << count << std::endl;
        
        auto result = run_fixedtime_single<shape>(params, time);
        auto score = result.score();

        if (score > hi_score)
        {
            hi_score = score;
        }
        
    }

    std::cout << "Loop finished" << std::endl;
    std::cout << "Score:                " << hi_score << std::endl;
    std::cout << std::endl;

    return hi_score;
};


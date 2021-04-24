#pragma once

#include <vector>

struct multisample_run_result
{
    long long sum_single_time;
    long long best_single_time;
    long long total_time;

    long long score() const
    {
        return 1e13 / total_time;
    }
};

//struct multisample_loop_result
//{
//    long long avg_time;
//    long long best_time;
//
//    long long score() const
//    {
//        return calc_score(best_time);
//    }
//};

struct fixedtime_run_result
{
    long long count;
    long long time;

    long long score() const
    {
        return count * 1e10 / time;
    }
};

struct run_params
{
    vectorization_level vec_level;

    long long samples;
    long long size;

    int threads;

    run_params(vectorization_level vec_level, long long samples, long long size, int threads)
        : vec_level(vec_level), samples(samples), size(size), threads(threads) {}
    
    long long total_calculations() const
    {
        return (samples * size) * (samples * size);
    }
};

//struct graph_loop_params
//{
//    graph_run_params run_params;
//    int loops;
//};

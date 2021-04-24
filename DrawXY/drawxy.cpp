#include <iostream>
#include <vector>
#include <numeric>
#include <chrono>

//#define PRINT_RESULT

#include "drawxy_common.h"
#include "drawxy_draw_funcs.h"
#include "drawxy_graph_funcs.h"
#include "drawxy_run.h"


int main()
{
    // Parameters
    
    int size = 32;

    const float scale_x = 4.0;
    const float scale_y = 4.0;
    const float offset_x = -2.0;
    const float offset_y = -2.0;
    const float threshold = 0.5;

    int loops = 10;
    int threads = 2;

    const graph_shape shape = graph_shape::CIRCLE;
    const vectorization_level vec_level = vectorization_level::AVX2;

    int size2 = size * size;

    // Input
    bench_type bt;

    int in;

    std::cout << "Enter benchmark type: ";
    std::cin >> in;
    bt = (bench_type) in;

    //std::cout << "Enter Graph Shape: " << std::flush;
    //std::cin >> in;
    //shape = (graph_shape) in;

    //std::cout << "Enter vectorization level: " << std::flush;
    //std::cin >> in;
    //vec_level = (vectorization_level) in;

    std::cout << std::endl;

    //double avg_time = 0.0;
    //double best_time = INFINITY;

    long long result_single;
    long long result_multi;

    if (bt == bench_type::MULTISAMPLE)
    {
        int samples = std::exp2(12);
        
        result_single = run_multisample_loop<shape>(run_params(vec_level, samples, size, 1), 5);

        result_multi = run_multisample_loop<shape>(run_params(vec_level, samples, size, threads), 5);

        
    }

    else if (bt == bench_type::FIXED_TIME)
    {
        int samples = std::exp2(12);

        result_single = run_fixedtime_loop<shape>(run_params(vec_level, samples, size, 1), 5000, 5);

        result_multi = run_fixedtime_loop<shape>(run_params(vec_level, samples, size, threads), 5000, 5);
    }
    
    std::cout << "All loops finished" << std::endl;
    std::cout << "ST Score:             " << result_single << std::endl;
    std::cout << "MT Score:             " << result_multi << std::endl;
    std::cout << "ST:MT Ratio:          " << ((double)result_multi / result_single) << std::endl;
}
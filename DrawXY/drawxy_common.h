#pragma once

#include <vectorclass.h>
#include <string>
#include <vector>

const Vec8f one_v(1);
const Vec8f none_v(-1);
const Vec8f zero_v(0);

const Vec8f ci_v8(0, 1, 2, 3, 4, 5, 6, 7);

enum class vectorization_level
{
    NONE,
    AVX2
};

inline std::string vec_level_to_string(vectorization_level obj)
{
    switch (obj)
    {
    case vectorization_level::NONE:
        return "No defined vectorization";
    case vectorization_level::AVX2:
        return "AVX2 (256 bit)";
    }
}

enum class graph_shape
{
    EMPTY,
    CIRCLE,
    HYPERBOLA,
    SQUARE
};

inline std::string graph_shape_to_string(graph_shape obj)
{
    switch (obj)
    {
    case graph_shape::EMPTY:
        return "NONE";
    case graph_shape::CIRCLE:
        return "Circle [x^2 + y^2 < 1]";
    case graph_shape::HYPERBOLA:
        return "Hyperbola [x^2 - y^2 < 1]";
    case graph_shape::SQUARE:
        return "Square [-1 < x < 1 && -1 < y < 1]";
    }
}

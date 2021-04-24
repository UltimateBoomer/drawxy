#pragma once

#include "drawxy_common.h"

//template <typename T> const T func_dummy(const T, const T);
//template <typename T> const T func_circle(const T, const T);
//template <typename T> const T func_hyperb(const T, const T);
//template <typename T> const T func_square(const T, const T);

template <typename T, graph_shape shape> const T draw_func(T, T);

template <> const float draw_func<float, graph_shape::EMPTY>(const float x, const float y)
{
    return 0.0f;
}

template <> const Vec8f draw_func<Vec8f, graph_shape::EMPTY>(const Vec8f x_v, const Vec8f y_v)
{
    return zero_v;
}

template <> const float draw_func<float, graph_shape::CIRCLE>(const float x, const float y)
{
    float n = x * x + y * y;
    return n < 1.0f ? 1.0f : 0.0f;
}

template <> const Vec8f draw_func<Vec8f, graph_shape::CIRCLE>(const Vec8f x_v, const Vec8f y_v)
{
    Vec8f r_v = square(x_v) + square(y_v);
    return select(r_v < one_v, one_v, zero_v);
}

template <> const float draw_func<float, graph_shape::HYPERBOLA>(const float x, const float y)
{
    float n = x * x - y * y;
    return n > 1.0f ? 1.0f : 0.0f;
}

template <> const Vec8f draw_func<Vec8f, graph_shape::HYPERBOLA>(const Vec8f x_v, const Vec8f y_v)
{
    Vec8f r_v = square(x_v) - square(y_v);
    return select(r_v < one_v, one_v, zero_v);
}

template <> const float draw_func<float, graph_shape::SQUARE>(const float x, const float y)
{
    return x > -1.0f && x < 1.0f && y > -1.0f && y < 1.0f ? 1.0f : 0.0f;
}

template <> const Vec8f draw_func<Vec8f, graph_shape::SQUARE>(const Vec8f x_v, const Vec8f y_v)
{
    Vec8fb mask_v = (x_v > none_v) && (x_v < one_v) && (y_v > none_v) && (y_v < one_v);

    return select(mask_v, one_v, zero_v);
}

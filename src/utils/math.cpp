#include "utils/math.hpp"
#include <cmath>
#include <cstdint>

// -------- Specializations of math_sqrt --------

template <>
float math_sqrt(float x)
{
    return sqrtf(x);
}

// Clearly not my code
template <>
int math_sqrt(int n)
{
    int g = 0x8000;
    int c = 0x8000;
    for (;;)
    {
        if (g * g > n)
        {
            g ^= c;
        }
        c >>= 1;
        if (c == 0)
        {
            return g;
        }
        g |= c;
    }
}

// -------- math_floor overloads --------

float math_floor(float x)
{
    return floorf(x);
}

double math_floor(double x)
{
    return floor(x);
}

// -------- math_mod overload (float) --------

float math_mod(float x, float m)
{
    return fmodf(fmodf(x, m) + m, m);
}

double math_mod(double x, double m)
{
    return fmodf(fmodf(x, m) + m, m);
}

// -------- vec_dist overloads (non‑template) --------

int vec_dist(const IVec &a, const IVec &b)
{
    return math_sqrt<int>((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

float vec_dist(const FVec &a, const FVec &b)
{
    return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

// -------- vec_floor overloads --------

sf::Vector2f vec_floor(const sf::Vector2f &v)
{
    return {floorf(v.x), floorf(v.y)};
}

sf::Vector2<double> vec_floor(const sf::Vector2<double> &v)
{
    return {floor(v.x), floor(v.y)};
}

// -------- tile <-> position helpers --------

IVec vec_pos_to_tile(const FVec &v)
{
    return IVec{(int)floorf(v.x), (int)floorf(v.y)};
}

FVec vec_tile_to_pos(const IVec &v)
{
    return (FVec)v + FVec{0.5f, 0.5f};
}

// -------- AABB helpers --------

bool aabb_intersect_circle(
    const FVec boxCenter,
    const FVec boxSize,
    const FVec circlePos,
    const float circleRadius)
{
    auto boxPos = boxCenter - boxSize / 2.f;
    float x = math_clamp(
        circlePos.x,
        boxPos.x,
        boxPos.x + boxSize.x);
    float y = math_clamp(
        circlePos.y,
        boxPos.y,
        boxPos.y + boxSize.y);
    float d = vec_distsq(circlePos, {x, y});
    return d <= circleRadius * circleRadius;
}

FVec aabb_closest_point(
    const FVec &boxCenter,
    const FVec &boxSize,
    const FVec &point)
{
    FVec start = boxCenter - boxSize / 2.f;
    FVec end = boxCenter + boxSize / 2.f;
    return {math_clamp(point.x, start.x, end.x),
            math_clamp(point.y, start.y, end.y)};
}

FVec delta_from_crect(
    const FVec &pos,
    float radius,
    const FVec &rectPos,
    FVec rectSize)
{
    FVec dist = pos - rectPos;
    FVec delta;
    rectSize.x += radius;
    rectSize.y += radius;
    if (fabs(dist.x) > fabs(dist.y))
        delta.x = ((dist.x > 0.0) ? 1 : -1) * rectSize.x / 2.f - dist.x;
    else
        delta.y = ((dist.y > 0.0) ? 1 : -1) * rectSize.y / 2.f - dist.y;
    return delta * 1.f;
}

bool aabb_collision_center(
    const FVec &centerA,
    const FVec &sizeA,
    const FVec &centerB,
    const FVec &sizeB)
{
    float dx, dy;
    dx = fabs(centerA.x - centerB.x);
    dy = fabs(centerA.y - centerB.y);
    return dx < (sizeA.x + sizeB.x) / 2.f &&
           dy < (sizeA.y + sizeB.y) / 2.f;
}

FVec aabb_center_to_corner(
    const FVec &centerA,
    const FVec &sizeA)
{
    return FVec{
        centerA.x - sizeA.x / 2,
        centerA.y - sizeA.y / 2};
}

float aabb_distance_rectangle(
    const FVec &centerA,
    const FVec &sizeA,
    const FVec &centerB,
    const FVec &sizeB)
{
    FVec dif = centerA - centerB;
    vec_abs(dif);
    dif -= sizeA / 2.f + sizeB / 2.f;
    return vec_len(FVec{
        math_max(0.f, dif.x),
        math_max(0.f, dif.y)});
}

float aabb_distance_point(
    const FVec &centerA,
    const FVec &centerB,
    const FVec &sizeB)
{
    FVec dif = centerA - centerB;
    vec_abs(dif);
    dif -= sizeB / 2.f;
    return vec_len(FVec{
        math_max(0.f, dif.x),
        math_max(0.f, dif.y)});
}

float aabb_distance_circle(
    const FVec &a,
    const float r,
    const FVec &b,
    const FVec &s)
{
    float d = aabb_distance_point(a, b, s) - r;
    return (d < 0.0f) ? 0.0f : d;
}
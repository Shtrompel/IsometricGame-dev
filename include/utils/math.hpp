#ifndef GAME_UTILS_MATH
#define GAME_UTILS_MATH

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <type_traits>

#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>

#include "../utils/globals.hpp"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif // __GNUC__

// -------- Template functions (kept inline in header) --------

template <typename U>
static inline U math_abs(const U x)
{
    return (x >= (U)0 ? (U)x : (U)-x);
}

template <typename U>
static inline U math_signalt(const U x)
{
    return (x >= (U)0 ? (U)1 : (U)-1);
}

template <typename U>
static inline U math_sign(const U x)
{
    if (x == (U)0)
        return (U)0;
    return math_signalt(x);
}

// Declaration of template; specializations are defined in .cpp
template <typename U>
U math_sqrt(U x);

// math_floordiv (template)
template <typename U>
inline U math_floordiv(U num, U den)
{
    static_assert(std::is_integral<U>::value, "math_floordiv only takes integers.");
    if (0 < (num ^ den))
        return num / den;
    else
    {
        auto res = div(num, den);
        return (res.rem) ? res.quot - 1
                         : res.quot;
    }
}

// math_floor for integral types (template)
template <typename U, typename std::enable_if_t<std::is_integral<U>::value, bool> = true>
constexpr U math_floor(const U &x)
{
    return x;
}

// Overloads for floating‑point types (declared, defined in .cpp)
float math_floor(float x);
double math_floor(double x);

// math_mod for integral types (template)
template <typename U>
inline U math_mod(U x, U m)
{
    static_assert(std::is_integral<U>::value, "math_mod only takes integers.");
    return ((x % m) + m) % m;
}

// math_mod for float (non‑template) – declared, defined in .cpp
float math_mod(float x, float m);

double math_mod(double x, double m);

// min/max/between/distsq/clamp/map (all templates)
template <typename U>
static inline U math_max(U a, U b)
{
    return (a > b) ? a : b;
}

template <typename U>
static inline U math_min(U a, U b)
{
    return (a < b) ? a : b;
}

template <typename U>
static inline bool math_between(const U &x, const U &a, const U &b)
{
    return a <= x && x < b;
}

template <typename U>
U math_distsq(U a, U b, U A, U B)
{
    return (a - A) * (a - A) + (b - B) * (b - B);
}

template <typename U>
U math_clamp(U x, U min, U max)
{
    return x < min ? min : (x > max ? max : x);
}

template <typename U = float>
static float math_map(U x, float a, float b, float A, float B)
{
    return (x - a) * (B - A) / (b - a) + A;
}

// vec_compare (template)
template <typename Vec>
inline bool vec_compare(const Vec &vecA, const Vec &vecB)
{
    if (vecA.y == vecB.y)
        return vecA.x < vecB.x;
    return vecA.y < vecB.y;
}

template <typename Vec>
static void vec_abs(Vec &vec)
{
    vec.x = math_abs(vec.x);
    vec.y = math_abs(vec.y);
}

// vec_dist – declared here; overloads for IVec and FVec are defined in .cpp
template <typename Vec>
inline int vec_dist(const Vec &a, const Vec &b)
{
    return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}
// Non‑template overloads (declared, defined in .cpp)
int vec_dist(const IVec &a, const IVec &b);
float vec_dist(const FVec &a, const FVec &b);

template <typename Vec>
inline float vec_dist_circles(
    const Vec &a,
    const float ra,
    const Vec &b,
    const float rb)
{
    float d = vec_dist(a, b) - ra - rb;
    return (d < 0.0f) ? 0.0f : d;
}

template <typename U>
static bool vec_inside(
    const U& pos,
    const U& rectCorner,
    const U& rectSize)
{
    return
        math_between(pos.x,
            rectCorner.x,
            rectCorner.x + rectSize.x)
        &&
        math_between(pos.y,
            rectCorner.y,
            rectCorner.y + rectSize.y);
}

template <typename U>
inline bool vec_inside(
    const sf::Vector2<U> &v, U x, U y, U w, U h)
{
    return vec_inside(v, {x, y}, {w, h});
}

template <typename U>
inline sf::Vector2<U> vec_reverse(const sf::Vector2<U> &v)
{
    return sf::Vector2<U>{v.y, v.x};
}

template <typename U>
inline sf::Vector2<U> vec_mult(const sf::Vector2<U> &v1, const U &x)
{
    return sf::Vector2<U>{v1.x * x, v1.y * x};
}

template <typename U>
inline sf::Vector2<U> vec_mult(const sf::Vector2<U> &v1, const sf::Vector2<U> &v2)
{
    return sf::Vector2<U>{v1.x * v2.x, v1.y * v2.y};
}

#define TYPE_T_U decltype(std::declval<T>() * std::declval<U>())

template <typename T, typename U>
inline sf::Vector2<TYPE_T_U> operator*(const sf::Vector2<T> &v1, const sf::Vector2<U> &v2)
{
    return sf::Vector2<TYPE_T_U>{v1.x * v2.x, v1.y * v2.y};
}

template <typename T, typename U>
inline sf::Vector2<TYPE_T_U> operator/(const sf::Vector2<T> &v1, const sf::Vector2<U> &v2)
{
    return sf::Vector2<TYPE_T_U>{v1.x / v2.x, v1.y / v2.y};
}

template <typename T, typename U>
inline sf::Vector2<TYPE_T_U> vec_mult(const sf::Vector2<T> &v1, const sf::Vector2<U> &v2)
{
    return sf::Vector2<TYPE_T_U>{v1.x * v2.x, v1.y * v2.y};
}

template <typename U = float>
static constexpr inline U vec_distsq(const Vec<U> &v1, const Vec<U> &v2)
{
    U dx = v2.x - v1.x, dy = v2.y - v1.y;
    return (dx * dx + dy * dy);
}

template <typename U = float>
inline bool vec_cmp_dist(const Vec<U> &posA, const Vec<U> &posB, const Vec<U> &center)
{
    return vec_distsq(posA, center) < vec_distsq(posB, center);
}

template <typename U = float>
static U vec_lensq(const Vec<U> &v)
{
    return (v.x * v.x + v.y * v.y);
}

template <typename U = float>
static U vec_lensq(const Vec<U> &&v)
{
    return (v.x * v.x + v.y * v.y);
}

template <typename U>
static U vec_len(const Vec<U> &vec)
{
    return (U)math_sqrt<U>(vec_lensq<U>(vec));
}

template <typename U>
static Vec<U> vec_from_angle(
    const float angle)
{
    float x = cos(angle);
    float y = sin(angle);
    return Vec<U>(x, y);
}

template <typename U>
static U vec_angle_between(
    const sf::Vector2<U> &vecA,
    const sf::Vector2<U> &vecB)
{
    U dot = vecA.x * vecB.x + vecA.y * vecB.y;
    U det = vecA.x * vecB.y - vecA.y * vecB.x;
    return (U)atan2(det, dot);
}

template <typename U>
static float vec_normalize(sf::Vector2<U> &vec)
{
    float len = vec_len<U>(vec);
    if (len == (U)0)
    {
        vec = {(U)0, (U)0};
        return (U)0;
    }
    vec /= len;
    return len;
}

template <typename U>
static void vec_limit(sf::Vector2<U> &vec, U limit)
{
    U len = math_sqrt<U>(vec_len<U>(vec));
    if (len > limit)
    {
        vec = vec / len * limit;
    }
}

template <typename T, typename U>
static sf::Vector2<U> vec_convert(const sf::Vector2<T> v)
{
    sf::Vector2<U> out = {(U)v.x, (U)v.y};
    return out;
}

template <typename U>
static inline void vec_frames_apply(sf::Vector2<U> &dest, const sf::Vector2<U> &source)
{
    dest.x = (source.x >= 0) ? dest.x : source.x;
    dest.y = (source.y >= 0) ? dest.y : source.y;
}

// vec_floor for integral types (template) – returns unchanged
template <typename U, typename std::enable_if_t<std::is_integral<U>::value, bool> = true>
static constexpr sf::Vector2<U> vec_floor(const sf::Vector2<U> &v)
{
    return v;
}

// vec_floor for floating‑point types (declared, defined in .cpp)
sf::Vector2f vec_floor(const sf::Vector2f &v);
sf::Vector2<double> vec_floor(const sf::Vector2<double> &v);

template <typename U>
static inline U vec_prod(const sf::Vector2<U> &x)
{
    return x.x * x.y;
}

template <typename U>
static inline U vec_sum(const sf::Vector3<U> &x)
{
    return x.x + x.y + x.z;
}

// Non‑template helper functions (declared, defined in .cpp)
IVec vec_pos_to_tile(const FVec &v);
FVec vec_tile_to_pos(const IVec &v);

bool aabb_intersect_circle(
    const FVec boxCenter,
    const FVec boxSize,
    const FVec circlePos,
    const float circleRadius);

FVec aabb_closest_point(
    const FVec &boxCenter,
    const FVec &boxSize,
    const FVec &point);

template <typename T>
Vec<T> vec_rotate_2d(const Vec<T>& v, float a )
{
    T x = v.x * cosf(a) - v.y * sinf(a);
    T y = v.x * sinf(a) + v.y * cosf(a);
    return { x, y };
}

template <typename T>
Vec<int> vec_90_rotate_axis(T x)
{
    const static Vec<int> table[] = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};
    return table[(int)math_mod(x, 4)];
}

template <typename T>
bool vec_90_rotate_reverse(T x)
{
    const static bool table[] = {false, true, false, true};
    return table[(int)math_mod(x, 4)];
}


// https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
static inline uint32_t log_2(uint64_t n)
{
	static const int table[64] = {
		0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61,
		51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62,
		57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56,
		45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5, 63};

	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;

	return table[(n * 0x03f6eaf2cd271461) >> 58];
}

template <typename T, typename U>
Vec<U> vec_90_rotate(const Vec<U> &v, T x)
{
    //return vec_mult(v, vec_90_rotate_axis(x));
    switch ((int)math_mod(x, 4))
    {
    case 0:
        return Vec<U>{v.x, v.y};
    case 1:
        return Vec<U>{v.y, -v.x};
    case 2:
        return Vec<U>{-v.x, -v.y};
    case 3:
        return Vec<U>{-v.y, v.x};
    }
    return {0, 0};
}

// Non‑template collision helpers (declared, defined in .cpp)
FVec delta_from_crect(
    const FVec &pos,
    float radius,
    const FVec &rectPos,
    FVec rectSize);

bool aabb_collision_center(
    const FVec &centerA,
    const FVec &sizeA,
    const FVec &centerB,
    const FVec &sizeB);

FVec aabb_center_to_corner(
    const FVec &centerA,
    const FVec &sizeA);

float aabb_distance_rectangle(
    const FVec &centerA,
    const FVec &sizeA,
    const FVec &centerB,
    const FVec &sizeB);

float aabb_distance_point(
    const FVec &centerA,
    const FVec &centerB,
    const FVec &sizeB);

float aabb_distance_circle(
    const FVec &a,
    const float r,
    const FVec &b,
    const FVec &s);

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

#endif // GAME_UTILS_MATH
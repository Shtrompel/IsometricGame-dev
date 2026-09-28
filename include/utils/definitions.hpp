
#ifndef _GAME_DEFINITIONS
#define _GAME_DEFINITIONS

#include <cstdint>
#include <cstdlib>
#include "nlohmann/json.hpp"


template <typename T>
using Vec = sf::Vector2<T>;

using FVec = Vec<float>;
using DVec = Vec<double>;
using IVec = Vec<int>;
using UVec = Vec<unsigned>;
using LVec = Vec<long>;
using ULVec = Vec<unsigned long>;

template <typename T>
struct Rect
{
	Vec<T> pos;
	Vec<T> size;
};

using FRect = Rect<float>;
using DRect = Rect<double>;
using IRect = Rect<int>;
using URect = Rect<unsigned>;
using LRect = Rect<long>;
using ULRect = Rect<unsigned long>;


typedef uint8_t t_byte;
typedef uint16_t t_count;
typedef t_byte t_material;
typedef uint32_t t_millis;

typedef int t_sprite;
typedef float t_seconds;
typedef unsigned t_use;
typedef size_t t_group;
typedef size_t t_id;

using t_jsonpack = std::unordered_map<std::string, nlohmann::json>;

#endif // _GAME_DEFINITIONS
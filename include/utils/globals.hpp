#ifndef _GAME_GLOBALS
#define _GAME_GLOBALS

#include <inttypes.h>
#include <SFML/System/Vector2.hpp>
#include <list>
#include <deque>
#include <unordered_map>
#include <string>

#include "utils/container/chunked_colony.hpp"
#include "utils/definitions.hpp"
#include "nlohmann/json.hpp"

#include <boost/container/stable_vector.hpp>

#ifdef __GNUC__

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-variable"

#endif //  __GNUC__



#define EMPTY_TILETREE (t_tiletree{ VEC_LIMIT, POINT_LIMIT })

#define PATHFIND_HEURISTIC_OCTILE


enum class GameScreen
{
	WINDOW_MENU,
	WINDOW_CAMPAIGN,
	WINDOW_ABOUT,
	WINDOW_GAME
};

//typedef std::pair<t_group, t_id> t_idpair;
struct IdPair
{
	t_group group;
	t_id id;
	
	bool operator<(const IdPair &other) const
	{
		if (this->group == other.group)
			return this->id < other.id;
		return this->group < other.group;
	}

	bool operator==(const IdPair &other) const
	{
		return this->group == other.group &&
			   this->id == other.id;
	}

	bool operator!=(const IdPair& other) const
	{
		return !operator==(other);
	}

	inline std::string to_string() const
	{
		std::string s = "{";
		s += std::to_string(group);
		s += ", ";
		s += std::to_string(id);
		s += "}";
		return s;
	}
};
typedef IdPair t_idpair;

const constexpr t_idpair IDPAIR_NONE = { 0, 0 };

struct SpawnInfo
{
	size_t serializableID = 0;
	t_idpair id = IDPAIR_NONE;

	std::string to_string()
	{
		std::string ret = "";
		ret += std::to_string(serializableID);
		ret += ", ";
		ret += id.to_string();
		return ret;
	}
};


// Object container type must have:
// Always valid iterators, even after change
// begin() and end()
// push_back()
template <class T>
using t_bodies_ctr = ChunkedColony<T>;
template <class T>
using t_bodies_itr = typename t_bodies_ctr<T>::iterator;

template <class T>
using t_builds_ctr = ChunkedColony<T>;
template <class T>
using t_builds_itr = typename t_builds_ctr<T>::iterator;

template <class T>
using t_entities_ctr = std::vector<T>;
template <class T>
using t_entities_itr = typename t_entities_ctr<T>::iterator;

template <class T>
using t_bbuilds_ctr = std::vector<T>;
template <class T>
using t_bbuilds_itr = typename t_bbuilds_ctr<T>::iterator;

// Enums

// Every emum is continuous to another so every enum
// will have a potential quad tree of the type it represents
// for efficiencie's sake.
// BuildingType quadTree is unimplemented!

typedef enum class ConstantNumeric
{
	MAX_BUILD_BINDED,
	A_STAR_GREED_VALUE,
	A_STAR_DIJKSTRA_VALUE,
	PATH_COUNT,
	COUNT
} t_constnum;

typedef enum class ConstantFloating
{
	MAX_NETWORK_RADIUS,
	PATH_WEIGHT_SUB,
	PATH_WEIGHT_BASE,
	DEFAULT_ENTITY_AWARENESS_RADIUS,
	CLOGGED_INVENTORY_PERCENTAGE,
	EMPTY_INVENTORY_PERCENTAGE,
	COUNT
} t_constflt;

typedef enum class ConstantBoolean
{
	GOD_MODE,
	ENABLE_CONSTRUCTIONS,
	COUNT
} t_constbool;

static const std::unordered_map<std::string, int>
	CONSTS_MAP =
		{
			{"MAX_BUILD_BINDED",
			 (int)ConstantNumeric::MAX_BUILD_BINDED},
			{"A_STAR_GREED_VALUE",
			 (int)ConstantNumeric::A_STAR_GREED_VALUE},
			{"A_STAR_DIJKSTRA_VALUE",
			 (int)ConstantNumeric::A_STAR_DIJKSTRA_VALUE},
			{"PATH_COUNT",
			 (int)ConstantNumeric::PATH_COUNT},

			{"MAX_NETWORK_RADIUS",
			 (int)ConstantFloating::MAX_NETWORK_RADIUS},
			{"PATH_WEIGHT_SUB",
			 (int)ConstantFloating::PATH_WEIGHT_SUB},
			{"PATH_WEIGHT_BASE",
			 (int)ConstantFloating::PATH_WEIGHT_BASE},
			 {"DEFAULT_ENTITY_AWARENESS_RADIUS",
			 (int)ConstantFloating::DEFAULT_ENTITY_AWARENESS_RADIUS},
			{"CLOGGED_INVENTORY_PERCENTAGE",
			 (int)ConstantFloating::CLOGGED_INVENTORY_PERCENTAGE},
			{"EMPTY_INVENTORY_PERCENTAGE",
			 (int)ConstantFloating::EMPTY_INVENTORY_PERCENTAGE},

			{"ENABLE_CONSTRUCTIONS",
			 (int)ConstantBoolean::ENABLE_CONSTRUCTIONS},
			 {"GOD_MODE",
			 (int)ConstantBoolean::GOD_MODE}
};


constexpr int DEBUG_SELECT_BUILDING_SPRITE = -1;
constexpr const char* DEBUG_SELECT_BUILDING_NAME = "";

// Constants

constexpr int STRCMP_EQUAL = 0;

constexpr size_t MAX_SHORT_STR = 32;
constexpr int INFINITE_LOOP = 99;
constexpr int SMALL_INFINITY = 99;
constexpr int BIG_INFINITY = 999;

// How far an entity need to be near other entity before going "follow" mode
constexpr float MIN_FOLLOW_DISTANCE = 0.8f;

constexpr float INFINITE_DISTANCE = -1.f;

constexpr int BUILDING_INFINITE_HEALTH = -1;

#if defined(__ANDROID__) || defined(TARGET_OS_IPHONE)
#define MODILE_DEVICE
#else
#endif

// Math Constants

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#define M_2PI 6.28318530717958647693

constexpr int ROTATION_0 = 0;
constexpr int ROTATION_90 = 1;
constexpr int ROTATION_180 = 2;
constexpr int ROTATION_270 = 3;
constexpr int ROTATION_INIT = ROTATION_0;

// Gameplay Constants
constexpr int POWER_SPEED = 10;
constexpr t_seconds COST_TIMER = 5.0f;
constexpr t_seconds MIN_TIME_PATHFIND = 0.05f;

static const sf::Vector2i DEFAULT_TILE_SIZE = {256, 128};
static const int DEFAULT_TILE_HEIGHT = 256;
constexpr int CHUNK_W = 32;
constexpr int CHUNK_H = 32;

constexpr float ENTITY_COLLISION_RADIUS = 0.5f / 4.f;
static const sf::Vector2f ENTITY_COLLISION_SIZE = {0.1f, 0.1f};
static const sf::Vector2f ENTITY_INTERACTION_SIZE = {0.35f, 0.35f};

// Todo, make entity stats ebalt to override those values
constexpr float MIN_FOLLOW_MELEE_DISTANCE = 0.1f;
constexpr float MIN_FOLLOW_RANGED_DISTANCE = 5.0f;

constexpr float BERSERK_ATTACK_MULTIPLIER = 1.5f;
// Applied to squared distance when enemies weigh targets
constexpr float BERSERK_TARGET_DISTANCE_SCALE = 0.25f;
// How often a building refreshes its aura, and how long a refresh lasts
constexpr float AURA_TICK_SECONDS = 0.5f;
constexpr float AURA_LEASE_SECONDS = 1.25f;

// Frames per second for animations with no velocity to drive them (work, hit)
constexpr float STATIONARY_ANIM_SPEED = 4.f;

static const sf::Vector2i DIRECTIONS[8] = {
	{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

constexpr float NULL_FLOAT = -1.f;
constexpr int NULL_INT = -1;
#define NULL_STR ""
const IVec NULL_IVEC = {-1, -1};

enum BOOL3 : int8_t
{
	NONE = -1,
	FALSE = 0,
	TRUE = 1
};

constexpr t_sprite SPRITE_NONE = -1;

#ifdef __GNUC__

#pragma GCC diagnostic pop

#endif // __GNUC__

#endif // GAME_GLOBALS
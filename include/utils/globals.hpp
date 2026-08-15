#ifndef _GAME_GLOBALS
#define _GAME_GLOBALS

#include <inttypes.h>
#include <SFML/System/Vector2.hpp>
#include <list>
#include <deque>
#include <unordered_map>
#include <string>

#include "utils/container/chunked_colony.hpp"
#include "nlohmann/json.hpp"

#include <boost/container/stable_vector.hpp>

#ifdef __GNUC__

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-variable"

#endif //  __GNUC__



#define EMPTY_TILETREE (t_tiletree{ VEC_LIMIT, POINT_LIMIT })

#define PATHFIND_HEURISTIC_OCTILE

// typedefs

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

typedef size_t t_globalenum;
typedef size_t t_build_enum;
typedef size_t t_group;
typedef size_t t_id;
typedef size_t t_alignment;
typedef size_t t_serializable;

using t_jsonpack = std::unordered_map<std::string, nlohmann::json>;


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
			 (int)ConstantBoolean::GOD_MODE},
			{"EMPTY_INVENTORY_PERCENTAGE",
			 (int)ConstantBoolean::ENABLE_CONSTRUCTIONS}
};


constexpr int DEBUG_SELECT_BUILDING_SPRITE = -1;
constexpr const char* DEBUG_SELECT_BUILDING_NAME = "";

constexpr t_group ENUM_NONE						= 0u;
constexpr t_group ENUM_BODY_TYPE				= 1u;
constexpr t_group ENUM_ENTITY_TYPE				= 2u;
constexpr t_group ENUM_ENEMY_TYPE				= 3u;
constexpr t_group ENUM_CITIZEN_JOB				= 4u;
constexpr t_group ENUM_BUILDING_TYPE			= 5u;
constexpr t_group ENUM_PROPERTY_BOOL			= 6u;
constexpr t_group ENUM_PROPERTY_NUM				= 7u;
constexpr t_group ENUM_INGAME_PROPERTIES		= 8u;
constexpr t_group ENUM_ALIGNMENT				= 9u;
constexpr t_group ENUM_ENTITY_PROPERTY_BOOL		= 10u;
constexpr t_group ENUM_ENTITY_PROPERTY_NUM		= 11u;
constexpr t_group ENUM_BULLET_TYPE				= 12u;
constexpr t_group ENUM_BULLET_PROPERTY_BOOL		= 13u;
constexpr t_group ENUM_BULLET_PROPERTY_NUM		= 14u;
constexpr size_t  ENUM_GROUP_COUNT				= 15u;

constexpr t_serializable SERIALIZABLE_NONE			= 0u;
constexpr t_serializable SERIALIZABLE_NETWORK		= 1u;
constexpr t_serializable SERIALIZABLE_BUILD_BODY	= 2u;
constexpr t_serializable SERIALIZABLE_BUILD_BASE	= 3u;
constexpr t_serializable SERIALIZABLE_PATH			= 4u;
constexpr t_serializable SERIALIZABLE_ENTITY		= 5u;
constexpr t_serializable SERIALIZABLE_CITIZEN		= 6u;
constexpr t_serializable SERIALIZABLE_ENEMY			= 7u;
constexpr t_serializable SERIALIZABLE_CHUNKS		= 8u;
constexpr t_serializable SERIALIZABLE_DATA			= 9u;
constexpr t_serializable SERIALIZABLE_CONSTRUCTION	= 10u;
constexpr t_serializable SERIALIZABLE_RENDERER		= 11u;
constexpr t_serializable SERIALIZABLE_BULLET		= 12u;
constexpr size_t SERIALIZABLE_ALL				= 13u;

constexpr t_alignment ALIGNMENT_NONE			= 0u;
constexpr t_alignment ALIGNMENT_NEUTRAL			= 1u;
constexpr t_alignment ALIGNMENT_FRIENDLY		= 2u;
constexpr t_alignment ALIGNMENT_ENEMY			= 3u;

/// <summary>
/// The relation between two objects. Where "None" is
/// undefined, "NEUTRAL" means both ignore each other
/// "Friendly" enabled friendly interactions and so "Enemies".
/// </summary>
constexpr t_alignment ALIGNMENTS_NONE			= 0u;
constexpr t_alignment ALIGNMENTS_NEUTRAL		= 1u;
constexpr t_alignment ALIGNMENTS_FRIENDLY		= 2u;
constexpr t_alignment ALIGNMENTS_ENEMIES		= 3u;

const inline char STR_LOWER_NONE[] = "none";

enum class NoneEnum : t_globalenum
{
	NONE = 0,
	LAST
};

enum class BodyType : t_globalenum
{
	NONE,
	ALL,
	BUILDING,
	ENTITY,
	BULLET,
	PARTICLE,
	LAST
};

enum class EntityType : t_globalenum
{
	NONE,
	CITIZEN,
	ENEMY,
	LAST
};

enum class EnemyType : t_globalenum
{
	NONE,
	BRUTE,
	HUNTER,
	LAST
};

using EntityEnemyType = EnemyType;

enum class CitizenJob : t_globalenum
{
	NONE,
	BUILDER,
	COURIER,
	MINER,
	GATHERER,
	ELECTRICIAN,
	MAKRKSMAN,
	SOLDIER,
	RANGER,
	TANK,
	HERO,
	MECHA_BOT,
	TEST01, // Pathfind test,
	TOWER_WATCHER,
	LAST
};

enum class BuildingType : t_globalenum
{
	EMPTY,					 // 0
	ROAD,					 // 1
	WALL,					 // 2
	HOME,					 // 3
	STORAGE,				 // 4
	LOGISTICS_CENTER,
	GENRATOR,				 // 
	CAPACITOR,				 // 
	MINE,					 // 
	MINERS_POST,			 // 
	TOWER,					 // 
	ARMORY,					 // 
	TEMPLE,					 // 
	GRAVEYARD,				 // 
	BOMB,					 // 
	CONSTRUCTION,			 // 
	CONSTRUCTION_DEPARTMENT, // 
	BUILDERS_GUILD,			 // 
	RAW_ORE,				 // 
	RAW_GEMS,				 // 
	RAW_BODIES,				 // 
	RAW_VARIOUS,			 // 
	PILE_ORE,
	PILE_GEMS, // 
	PILE_BODIES,
	PILE_VARIOUS, // 24
	ENEMY_SPAWN,  // 25
	LAST
};

enum class PropertyBool : t_globalenum
{
	NONE,
	BARRIER,
	HOME,
	HARVESTABLE,
	STORAGE, // Designated storage
	TMP_STORAGE, // Stores stuff temporarily
	ANY_STORAGE, // All types of storage
	INSIDE,
	WORKPLACE,
	POWER_NETWORK,
	HIDDEN,
	UNREMOVABLE,
	UNFRIENDLY,
	FRIENDLY,
	HITABBLE,
	OFFENSIVE,
	LAST
};

enum class PropertyNum : t_globalenum
{
	NONE,
	SPEED_BONUS,
	DECAY_AMOUNT,
	STORAGE_FRAMES,
	POWER_FRAMES,
	ENTITY_FRAMES,
	ACTION_TIME,
	WORK_EFFICIENCY,
	PATH_WEIGHT_MULT,
	LAST
};

enum class IngameProperties : t_globalenum
{
	NONE,
	NEEDS_WORKERS,
	LAST
};

enum class EnumAlignment : t_globalenum
{
	NONE,
	NEUTRAL,
	FRIENDLY,
	ENEMY,
	LAST
};

enum class EntityPropertyBools: t_globalenum
{
	NONE,
	CAN_PHASE,
	IGNORE_ENTITIES,
	IGNORE_BUILDINGS,
	MELEE, // Entity goes and attacks enemies
	RANGED, // Entity shoots at entities
	LAST
};

enum class EntityPropertyNums : t_globalenum
{
	NONE,
	PATH_WEIGHT_BASE,
	PATH_WEIGHT_SUB,
	PATH_COUNT,
	LAST
};

enum class BulletType : t_globalenum
{
	NONE,
	DEFAULT,
	ENEMY_BULLET_01,
	LAST
};

enum class BulletPropertyBools : t_globalenum
{
	NONE,
	HOMING,
	BOUNCING,
	DESTRUCTIVE,
	SPECTRAL,
	LAST
};

enum class BulletPropertyNums : t_globalenum
{
	NONE,
	LAST
};

enum class GameMode
{
	VIEW,
	BUILD,
	DELETE,
	UPGRADE,
	DRAW
};

enum class ShapeType
{
	NONE = 0,
	POINT = 1,
	RECT = 2,
	CIRCLE = 3
};


enum class TextureOrigin
{
	NONE,
	CENTERED,
	BOTTOM,
	TOP_LEFT,
	POINT
};


constexpr size_t ENUM_COUNT = (size_t)IngameProperties::LAST;

constexpr size_t COUNT_PROPERTY_NUM =
	(t_globalenum)PropertyNum::LAST -
	(t_globalenum)PropertyNum::NONE;

constexpr size_t COUNT_PROPERTY_BOOL =
	(t_globalenum)PropertyBool::LAST -
	(t_globalenum)PropertyBool::NONE;

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

static const sf::Vector2i DIRECTIONS[8] = {
	{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

enum BOOL3 : int8_t
{
	NONE = -1,
	FALSE = 0,
	TRUE = 1
};

#ifdef __GNUC__

#pragma GCC diagnostic pop

#endif // __GNUC__

#endif // GAME_GLOBALS
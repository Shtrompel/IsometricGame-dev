#ifndef _BUILDING_ENUMS
#define _BUILDING_ENUMS

#include "utils/definitions.hpp"

typedef size_t t_alignment;
typedef size_t t_serializable;

typedef size_t t_globalenum;
typedef size_t t_build_enum;


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
constexpr t_group ENUM_BUILDING_VISUAL_TYPE		= 15u;
constexpr size_t  ENUM_GROUP_COUNT				= 16u;

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
	CONSTRUCTION,	
	ROAD,
	WALL,
	HOME,
	STORAGE,
	LOGISTICS_CENTER,
	CONSTRUCTION_DEPARTMENT,
	GENERATOR,			
	CAPACITOR,			
	MINE,				
	MINERS_POST,			
	TOWER,					
	ARMORY,					
	TEMPLE,
	LIGHT_TRAP,	
	GRAVEYARD,				
	BOMB,
	ENEMY_SPAWN, 
	RAW_ORE,				
	RAW_GEMS,				
	RAW_BODIES,				 
	RAW_VARIOUS,			 
	PILE_ORE,
	PILE_GEMS,
	PILE_BODIES,
	PILE_VARIOUS,
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
	ENTITY_HP_OVERRIDE,
	ENTITY_HP_RATIO,
	ENTITY_ATTACK_OVERRIDE,
	ENTITY_ATTACK_RATIO,
	ENTITY_SPEED_RATIO,
	ENTITY_ACTION_TIME_RATIO,
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
	ENEMY_BULLET_HOMING,
	HUNTER_BULLET_BASIC,
	HUNTER_BULLET_BETTER,
	HUNTER_BULLET_BEST,
	TOWER_BULLET_BASIC,
	TOWER_BULLET_BETTER,
	TOWER_BULLET_HOMING,
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

#endif // _BUILDING_ENUMS
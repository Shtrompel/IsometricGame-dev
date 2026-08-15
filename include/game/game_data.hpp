#ifndef _GAME_DATA
#define _GAME_DATA

// #define QUAD_TREE_DEBUG
#define QUAD_TREE_PRINT
#define GAME_TREE_ASSERTS

#include "utils/globals.hpp"
#include "utils/container/quad_tree.hpp"
#include "game_buildings.hpp"
#include "game_entity.hpp"
#include "game_grid.hpp"
#include "game_bullet.hpp"

#include "game/scenario/Timeline.hpp"

#include <cfloat>
#include <tuple>
#include <set>

struct QueryPoolPath;

// How small can a quadrent get in the quad tree
const static sf::Vector2i VEC_LIMIT = {1, 1};
// How many points can a quadtree hold before it splits
const static size_t POINT_LIMIT = 8;

typedef ChunkQuadTree<GameBody *> t_tiletree;

[[deprecated]] typedef std::pair<t_globalenum, t_tiletree *> t_tree_pair;

typedef std::pair<t_idpair, t_tiletree *> t_idwtree;
typedef EnumManager<t_tiletree, ENUM_GROUP_COUNT> t_enum_tree;
typedef EnumManager<UpgradeTree, 1> t_enum_upgrade;
typedef std::map<t_id, GameBodyConfig *> t_id_config;
typedef NestedMap<
	std::map,
	t_group,
	t_id,
	t_id_config>
	t_configmap;

struct PathData;
struct QueryThreadPool;

static bool debug_entity_check_home(EntityBody *entity)
{
	if (entity->entityType != EntityType::CITIZEN)
		return true;
	EntityCitizen *citizen = (EntityCitizen *)entity;
	if (!citizen->home)
		return true;

	BuildingBase *home;
	home = citizen->home;
	auto itr = std::find(
		home->entities.begin(),
		home->entities.end(),
		citizen);

	return itr != home->entities.end();
}

static bool debug_home_check_entities(BuildingBase *home)
{
	for (size_t i = 0; i < home->entities.size(); ++i)
	{
		EntityBody *entity = home->entities[i];
		if (entity->entityType != EntityType::CITIZEN)
			continue;
		EntityCitizen *citizen = (EntityCitizen *)entity;
		if (citizen->home != home)
		{
			return false;
		}
	}
	return true;
}

/**
 * To avoid buildings adding entities by themselfs,
 * transer the building to a queue and add the entities
 * after update function
 */
struct BuildingQueueData
{
	sf::Vector2i pos;
	UpgradeTree *step = nullptr;
	bool useResources = true;
	bool isConstruction = false;
	t_build_enum buildType;
};

struct BodyQueueData
{
	SpawnInfo spawn{};
	FVec pos{0.f, 0.f};
	FVec vel{0.f, 0.f};
	FVec dif{0.f, 0.f};
	GameBody *target = nullptr;
	t_alignment alignment = ALIGNMENT_NONE;
	BuildingBase *home = nullptr;
};

/*
	Should be part of BuildingBase class
	for "CONSTRUCTION" buildings.
	But i feel it's already overbloated.
	*/
struct ConstructionData : public Variant
{
	BuildingQueueData queueData;
	int work = 0;

	ConstructionData();

	ConstructionData(const BuildingQueueData &data, size_t id);

	int work_needed() const;

	int work_left() const;
};

void to_json(json &j, const ConstructionData &v);

void from_json(const json &j, ConstructionData &v);

struct Constructions : Variant
{
	template <class T, class U>
	using t_map = std::unordered_map<T, U>;

	struct MyCmp
	{
		bool operator()(
			const VariantPtr<BuildingBase> &lhs,
			const VariantPtr<BuildingBase> &rhs) const
		{
			// if (lhs.objectType == rhs.objectType)
			//	return lhs.objectId < rhs.objectId;
			// return lhs.objectType < rhs.objectType;
			bool a = lhs.t < rhs.t;
			return a;
		}
	};

	std::vector<std::pair<VariantPtr<BuildingBase>, size_t>>
		keysTmp;

	t_map<size_t, ConstructionData> container;
	t_map<VariantPtr<BuildingBase>, size_t> keys;
	typedef decltype(keys.begin()) iterator;

	size_t counter = 0;

	void clear();

	iterator begin();

	iterator end();

	iterator find(BuildingBase *b);

	size_t count(BuildingBase *b);

	ConstructionData &at(size_t i);

	void insert(BuildingBase *b, const ConstructionData &d);

	ConstructionData &at(BuildingBase *b);

	bool erase_value(BuildingBase *b);

	json to_json() const override;

	void from_json(const json &j) override;

	void serialize_publish(const SerializeMap &map) override;

	void serialize_initialize(const SerializeMap &map) override;
};

struct BodiesContext
{
	// GameBody lists
	// I don't need random access
	// And also every object keeps an iterator for later deletion
	t_bodies_ctr<GameBody *> bodies;
	t_builds_ctr<VariantPtr<BuildingBody>> buildings;
	t_entities_ctr<VariantPtr<EntityCitizen>> entityCitizens;

	t_bbuilds_ctr<BuildingBase *> buildingBases;
};

struct AppendingChangesContext
{
	// Appending changes to the world

	std::deque<BuildingBase *> entityQueue;
	std::deque<BuildingQueueData> buildingQueue;
	std::vector<BodyQueueData> bodyQueue;
	std::deque<sf::Vector2i> addedTiles;
	std::deque<sf::Vector2i> removedTiles;

	std::set<EntityBody *> entityRemoveQueue;

	void clear()
	{
		entityQueue.clear();
		buildingQueue.clear();
		bodyQueue.clear();
		addedTiles.clear();
		removedTiles.clear();

		entityRemoveQueue.clear();
	}
};

struct CounterContext
{
	VariantFactory variantFactory{};

	// Counters
	size_t genIndex = 0;
	// When an object is added it needs to have an unique id higher then the last added object
	// This map tracks the highest id number of each variant object type
	ClassIdCounter objectIdCounter{&variantFactory};
};

struct ResourceContext
{
	Resources resources;
	t_weights weights;
	int power = 0;

	// Connected electricity grids
	std::vector<VariantPtr<PowerNetwork>> networks;
	int lpowerTotal = 0;
	int powerTotal = 0;
};

struct GameData : public Variant
{
	// The defualt lambda for conditions, enables every input
	struct DEFAULT_CONDITION
	{
		constexpr bool operator()(const sf::Vector2i &, const GameBody *, const int) const
		{
			return true;
		}
	};
	typedef DEFAULT_CONDITION t_def_condition;

	// The defualt lambda for addition to pool of items condition, adds every item on default
	struct DEFAULT_ADDITION
	{
		constexpr bool operator()(const GameBody *, const int, const int) const
		{
			return true;
		}
	};
	typedef DEFAULT_ADDITION t_def_addition;

	//  The defualt lambda for comparation, always chooses the closer to the "center" variable
	struct DEFAULT_COMPARATOR
	{
		FVec center;

		DEFAULT_COMPARATOR(const FVec &center = {0.0f, 0.0f})
		{
			this->center = center;
		}

		inline bool operator()(
			const IVec &,
			const IVec &,
			const GameBody *a,
			const IVec &,
			const GameBody *b) const
		{
			return vec_distsq(center, a->pos) < vec_distsq(center, b->pos);
		}
	};
	typedef DEFAULT_COMPARATOR t_def_comparator;

	GameData();

	GameData(Chunks *chunks);

	~GameData();

	void clean_world();

	bool validate_state();

	void resolve_invalid_state();

	json to_json() const override;

	void from_json(const json &j) override;

	void serialize_publish(const SerializeMap &map) override;

	void serialize_initialize(const SerializeMap &map) override;

	std::list<t_idwtree> get_trees(GameBody *body);

	t_tiletree *get_tree(
		const t_group group,
		const t_id id);

	inline t_idwtree get_pair(
		const t_group group,
		const t_id id);

	// Defined in main, bad practice? Who caaaarres.
	t_seconds get_time();

	// Timer managment

	t_body_timer create_timer(t_seconds start);

	void add_timer(t_body_timer &timer);

	//

	std::string enum_get_str(const t_idpair &pair, bool outShort = false);

	std::string enum_get_str(t_group group, t_id id, bool outShort = false);

	std::string enum_get_str(const GameBody *g, bool outShort = false);

	static int alignment_compare(const size_t alignmentA, const size_t alignmentB);

	static int alignment_opposite(const size_t alignmentA, const size_t alignmentB);

	static size_t alignment_opposite(size_t alignment);

	// Depth sort for rendering
	void depth_sort(int rotation);

	// Resource

	void calculate_resources();

	std::string resources_to_str(Resources &res);

	// Body

	static float bodies_distance(GameBody *a, GameBody *b);

	static void damage_body(GameBody *a, float damage);

	bool body_is_collision(const sf::Vector2f &, const sf::Vector2f &size, sf::Vector2i *tilePosPtr = nullptr);

	void queue_citizen(BuildingBase *build);

	bool queue_add_build(const sf::Vector2i &, UpgradeTree *, bool = true, bool = true);

	bool queue_add_build(const sf::Vector2i &, const std::string &, bool = true, bool = true);

	bool queue_add_build(const sf::Vector2i &, BuildingType, bool = true, bool = true);

	bool queue_add_body(BodyQueueData &);

	void delete_game_body_generic(GameBody *gb);

	GameBody *add_game_body(
		const size_t,
		const t_group,
		const t_id,
		const FVec &);

	GameBody *add_game_body(
		const size_t,
		GameBodyConfig *,
		const FVec &);

	void print_body_config_map()
	{
		std::stringstream ss;
		ss << "Body config map:\n";

		for (auto &group : bodyConfigMap)
		{
			for (auto &type : group.second)
			{
				t_group typeId = type.first;
				std::string str = enum_get_str(group.first, type.first);
				ss << std::to_string(group.first) << " " << std::to_string(type.first);
				ss << " ";
				ss << str.c_str();
				ss << '\n';
			}
		}
		LOG("%s", ss.str().c_str());
	}

	std::string spawn_info_to_string(const SpawnInfo &spawnInfo);

	// Straightforward and primitive SpawnInfo constructor
	SpawnInfo spawn_info_get(size_t serialFizableId, const t_idpair &pair);

	// Building

	bool can_build(const BuildingQueueData &queue);

	t_bbuilds_itr<BuildingBase *> delete_building(BuildingBase *build);

	t_bbuilds_itr<BuildingBase *> delete_building(const sf::Vector2i &pos);

	bool confirm_building_base(BuildingBase *build);

	BuildingBase *add_building(
		const BuildingQueueData &queue,
		bool updatePath = true);

	BuildingBase *add_building(
		const sf::Vector2i &,
		UpgradeTree *,
		bool useResources = true);

	BuildingBase *add_building(
		const sf::Vector2i &,
		const std::string &,
		bool useResources = true);

	BuildingBase *add_building(
		const sf::Vector2i &,
		BuildingType,
		bool useResources = true);

	bool is_barrier(const sf::Vector2i &pos);

	// Entity

	bool get_free_neighbor(sf::Vector2f &outPos, const FVec &pos);

	EntityCitizen *add_entity_citizen(
		const FVec &pos,
		EntityStats *stats);

	EntityCitizen *add_entity_citizen(
		BuildingBase *build);

	BulletBody *add_bullet(
		const FVec &pos,
		const FVec &dir,
		EntityStats *stats = nullptr);

	EntityStats *get_entity_stats(
		const std::string &name,
		bool *success = nullptr);

	EntityStats *get_entity_stats(
		t_group group = ENUM_CITIZEN_JOB,
		t_id type = (t_id)CitizenJob::NONE,
		bool *success = nullptr);

	template <class T>
	T *get_generic_stats(
		t_group group,
		t_id type,
		bool *success = nullptr)
	{
		static_assert(
			std::is_base_of<GameBodyConfig, T>::value,
			"Template must be derived from GameBodyConfig");

		if (!bodyConfigMap.count(group, type))
		{
			if (success)
				*success = false;
			return nullptr;
		}
		if (success)
			*success = true;
		return dynamic_cast<T *>(
			bodyConfigMap.at(group, type)[0]);
	}

	void delete_entity(EntityBody *entity);

	[[deprecated]] EntityEnemy *add_entity_enemy(
		const sf::Vector2f &pos,
		t_sprite sprite = -1);

	EntityEnemy *add_entity_enemy(
		const FVec &pos,
		EntityStats *stats);

	void move_entity(
		EntityBody *entity,
		const sf::Vector2f &posA,
		const sf::Vector2f &posB);

	void move_body(
		GameBody *body,
		const sf::Vector2f &posA,
		const sf::Vector2f &posB);

	void show_entity(EntityBody *);

	void hide_entity(EntityBody *);

	// Nearest Neighrbor

	// Bodies

	template <class Type = GameBody, class Condition = t_def_addition>
	std::list<Type *>
	nearest_bodies_radius(
		const sf::Vector2f &pos,
		float radius,
		size_t limit = SIZE_MAX,
		Condition &&additionCondition = DEFAULT_ADDITION())
	{
		auto less = [pos](const GameBody *a, const GameBody *b)
		{
			const sf::Vector2f &posA = a->pos, &posB = b->pos;
			return vec_distsq(posA, pos) < vec_distsq(posB, pos);
		};
		std::list<Type *> ret;

		float radiusSq = radius * radius;
		int x1 = (int)math_floor(pos.x - radius), x2 = (int)math_floor(pos.x + radius);
		int y1 = (int)math_floor(pos.y - radius), y2 = (int)math_floor(pos.y + radius);
		for (int i = x1; i <= x2; ++i)
		{
			for (int j = y1; j <= y2; ++j)
			{
				if (!this->chunks->has_tile(i, j))
					continue;
				Tile &tile = this->chunks->get_tile(i, j);
				BuildingBody *build = tile.building;
				GameBody *body;

				if (build)
				{
					// float dist = vec_distsq(build->pos, pos);
					body = dynamic_cast<GameBody *>(build);
					assert(body);

					bool intersect = aabb_intersect_circle(
						build->pos,
						FVec{1.f, 1.f},
						pos,
						radius);

					if (!intersect ||
						build->pos == pos)
						continue;

					if (!additionCondition(
							body,
							i, j))
						continue;

					ret.insert(std::lower_bound(
								   ret.begin(),
								   ret.end(),
								   tile.building,
								   less),
							   dynamic_cast<Type *>(body));
				}
				if (!tile.is_barrier())
				{
					for (auto itr = tile.entities.begin(); itr != tile.entities.end(); ++itr)
					{
						EntityBody *e = *itr;
						body = dynamic_cast<GameBody *>(e);
						assert(body);
						float dist = vec_distsq(e->pos, pos);

						if (dist > radiusSq ||
							e->pos == pos ||
							dist == 0.0f)
							continue;

						if (!additionCondition(
								dynamic_cast<GameBody *>(body),
								i, j))
							continue;

						ret.insert(std::lower_bound(
									   ret.begin(),
									   ret.end(),
									   body,
									   less),
								   dynamic_cast<Type *>(body));
					}
				}
			}
		}

		if (ret.size() > limit)
			ret.erase(std::next(ret.begin(), limit), ret.end());
		return ret;
	}

	// Entity

	std::list<EntityBody *>
	nearest_entities_radius(
		const sf::Vector2f &,
		float,
		EntityType,
		size_t = -1);

	std::list<BuildingBody *>
	nearest_buildings_aabb(
		const sf::Vector2f &,
		float,
		size_t = -1);

	// Building

	std::list<BuildingBody *>
	nearest_buildings_radius(
		const sf::Vector2f &,
		float,
		size_t = -1);

	std::list<BuildingBody *> nearest_buildings_aabb(
		const sf::Vector2f &pos,
		const sf::Vector2f &size,
		float,
		size_t = -1);

	template <
		class Condition = t_def_condition>
	std::list<BuildingBody *>
	nearest_bodies_quad(
		const sf::Vector2f &pos,
		size_t limit = t_tiletree::MAX_SIZE,
		t_idpair treeId = {ENUM_BODY_TYPE, (t_id)BodyType::BUILDING},
		const Condition &condition = DEFAULT_CONDITION())
	{
		return nearest_bodies_quad(
			pos,
			limit,
			treeId,
			condition,
			DEFAULT_COMPARATOR(pos));
	}

	template <
		class Condition = t_def_condition,
		class Comparator = t_def_comparator>
	std::list<BuildingBody *>
	nearest_bodies_quad(
		const sf::Vector2f &pos,
		size_t limit,
		t_idpair treeId,
		const Condition &condition,
		const Comparator &comparator)
	{
		std::list<BuildingBody *> ret;
		auto insertor = std::back_inserter(ret);

		t_tiletree *tree = get_tree(treeId.group, treeId.id);

		tree->nearest_k_insertor<decltype(insertor), BuildingBody *>(
			vec_pos_to_tile(pos),
			limit,
			insertor,
			condition,
			comparator);

		return ret;
	}

	template <class Condition = t_def_condition, typename EnumType = BodyType>
	std::list<BuildingBody *>
	nearest_buildings_radius_quad(
		const sf::Vector2f &pos,
		size_t limit = t_tiletree::MAX_SIZE,
		int maxRadius = t_tiletree::MAX_NUM,
		t_idpair treeId = {ENUM_BODY_TYPE, (t_id)BodyType::BUILDING},
		Condition condition = DEFAULT_CONDITION())
	{
		std::list<BuildingBody *> ret;
		auto insertor = std::back_inserter(ret);

		t_tiletree *tree = get_tree(treeId.group, treeId.id);
		assert(tree);
		tree->nearest_k_radius_insertor<decltype(insertor), BuildingBody *>(
			vec_pos_to_tile(pos),
			limit,
			maxRadius,
			insertor,
			condition);
		return ret;
	}

	inline float get_const(const ConstantFloating &x)
	{
		return constFloating[(size_t)x];
	}

	inline float get_const(const ConstantNumeric &x)
	{
		return (float)constNumeric[(size_t)x];
	}

	std::list<BuildingBody *>
	nearest_bodies_quad(const sf::Vector2f &pos, size_t limit = -1);

public:
	// Readonly, once initialized don't change
	//
	// const CitizenJob defaultJob = CitizenJob::NONE;

	long frameCount = 0;

	Chunks *chunks = nullptr;
	Timeline timeline;
	GameTimerManager<t_seconds> timerManager;

	std::array<float, (size_t)ConstantFloating::COUNT>
		constFloating = {};
	std::array<int, (size_t)ConstantNumeric::COUNT>
		constNumeric = {};
	std::array<bool, (size_t)ConstantBoolean::COUNT>
		constBoolean = {};

	ResourceContext resourceContext;
	BodiesContext bodiesContext;
	AppendingChangesContext changesContext;
	CounterContext counterContext;

	Constructions constructions;

	// UI data
	BuildingBase *infoBuild = nullptr;
	EntityBody *infoEntity = nullptr;

	// Id Pair -> BodyConfig
	t_configmap bodyConfigMap;

	// Don't access directly, you buttocks.
	// used mostly by the get_tree and get_trees function.
	// Id Pair -> string + t_tree
	t_enum_tree mapEnumTree;

	// Protecting the trees in multithreading
	mutable std::recursive_mutex quadTreeMutex;
	QueryPoolPath *threadPath = nullptr;
};

// std::list<sf::Vector2i> generate_path(const sf::Vector2i &start, const sf::Vector2i &end, const Chunks *chunks);

#define BRUTE_FORCE0

#endif // _GAME_DATA

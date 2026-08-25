#ifndef _GAME_ENTITY
#define _GAME_ENTITY

#include "game_body.hpp"
#include "../file/serialization.hpp"

#include <set>
#include <deque>

struct BuildingBase;
struct BuildingBody;
struct Tile;
struct GameData;
struct PathData;

template <class Out>
struct QueryThreadInstance;

struct PathData : public Variant
{
	struct IndexVec
	{
		int index;
		sf::Vector2i value;

		bool operator<(const IndexVec &other)
		{
			return this->index < other.index;
		}
	};

	typedef std::deque<IndexVec> t_vec_list;

	t_vec_list path;
	t_vec_list::const_iterator follower;
	Tile *destination = nullptr;
	Vec<int> destPos;

	PathData(size_t id = 0);

	~PathData() {}

	bool valid() const;

	inline bool finished() const;

	std::string to_string() const;

	std::string why_invalid() const;

	sf::Vector2i front() const;

	void pop();

	void clear();

	void append(const PathData& pathData);

	// Json stuff

	json to_json() const override;

	void from_json(const json &j) override;

	void serialize_publish(const SerializeMap &map) override;

	void serialize_initialize(const SerializeMap &map) override;
};

 void to_json(json& j, const PathData& v);

 void from_json(const json& j, PathData& v);

typedef int EntityStatTags;
typedef int EntityStatProperty;
typedef int t_entity_type;
typedef std::pair<t_alignment, t_entity_type>
	t_entitystat_key;

 void to_json(json& j, const t_idpair& v);

 void from_json(const json& j, t_idpair& v);

 void to_json(json& j, const SpawnInfo& v);

 void from_json(const json& j, SpawnInfo& v);

struct EntityStats : GameBodyConfig
{
	// ID of the stats, as shown in the json file
	int id = 0;

	t_idpair type = {0, 0};
	int hp = -1;
	float radius = 0.0f;
	t_alignment alignment = 0;

	t_idpair spawnType = IDPAIR_NONE;
	size_t spawnID = 0;

	PropertySet<EntityPropertyBools, EntityPropertyNums> props;

	int inventorySize = 0;
	int transferSize = 0;
	float maxSpeed = 0.0f;
	float maxForce = 0.0f;
	t_idpair target = {0, 0};
	t_sprite spriteWalk = 0;
	t_sprite spriteAction = 0;
	t_sprite spriteHit = 0;
	float cooldownAction = 0.0f;
	float cooldownPath = 0.0f;

	bool aggressive = false;

	int sfxAction = 0;
	int sfxIdle = 0;
	int sfxHit = 0;
	int sfxDeath = 0;

	nlohmann::json to_json() const;

	std::string to_string() override;

};

struct EntityStatsContainer
{
	std::map<t_idpair, t_id> keys;
	std::map<t_id, EntityStats*> statMap;
	EntityStats* nullValue = nullptr;

	typedef decltype(statMap.begin()) iterator;

	EntityStatsContainer();
	
	~EntityStatsContainer();

	void insert(
		t_idpair pair,
		t_id key,
		const EntityStats& stats);

	EntityStats* operator[](t_id key);

	EntityStats* get(t_idpair pair);

	bool has(t_id key) const;

	bool has(t_idpair pair) const;

	size_t size() const;

	iterator begin();

	iterator end();
};

namespace EntityBodyStr
{
	const std::string ACTION_STRS[] = {
						"Doing nothing",
						"Idling",
						"Working",
						"Going to #destination",
						"Transfering resources",
						"Collecting resources",
						"Building",
						"Getting attacked",
						"Attacking",
						"Following #destination",
						"Shooting",
						"Getting a job"
	};
};

struct EntityBody : public GameBody
{
	enum class Action : uint8_t
	{
		NONE = 0,
		// No job, the citizen is walking around
		IDLE = 1,
		// The citizen is working inside it's workplace
		WORK = 2,
		// The citizen is going from one place to another
		MOVE = 3,

		TRANSFER = 4,
		COLLECT = 5,
		BUILD = 6,
		ATTACKED = 7,
		ATTACK = 8,
		FOLLOW = 9,
		SHOOT = 10,
		// Unique action for when an entity gets inside a workplace for the first time
		GET_JOB = 11
	};

	EntityType entityType = EntityType::NONE;
	int id = 0;
	t_sprite spriteWalk = 0;
	t_sprite spriteAction = 0;
	t_sprite spriteHit = 0;
	t_sprite spriteAttack = 0;

	// Mutable
	int hp = 2u;
	int attack = 1u;

	t_seconds animTime = 0.0f;
	std::list<VariantPtr<BuildingBody>> nearbyBuilds;
	PathData pathData;
	QueryThreadInstance<PathData> *buildFind = nullptr;

	int inventorySize = 60;
	int transferSize = 10;
	float maxSpeed = 1.5f;
	float maxForce = 0.9f, speedMultiplier = 0.05f;

	bool aggressive = false;
	std::map<t_idpair, float> targetPriority;
	std::map<t_idpair, float> targetBonus;

	// If there is a window containing this entitie's info,
	// make the game know to update the window
	bool updateInfo = false;

	// The last tile position of the target
	// If changes, update the path
	IVec targetLPos{};

	PropertySet<EntityPropertyBools, EntityPropertyNums>
		props;

	t_body_timer timerAction;
	t_body_timer timerPath;
	t_body_timer localClock;
	Action action = Action::IDLE;
	SpawnInfo spawn{};

	virtual ~EntityBody() = default;

	EntityBody() {}

	EntityBody(
		GameData *,
		const sf::Vector2f &pos,
		t_sprite spriteHolder,
		size_t,
		size_t);

	//virtual void apply_stats(const EntityStats &);
	
	void apply_data(GameBodyConfig* config) override;

	virtual void apply_data_derived(GameBodyConfig* config);
	
	bool confirm_body(GameData* context) override;
	
	virtual bool confirm_body_derived(GameData* context);
	
	bool search_entity();

	void go_to_building(PathData& path);

	int get_hp() const override;

	int& get_hp() override;

	bool set_path(PathData &&path);

	void update_path();

	void update(float delta) override;

	void change_action(Action action);

	virtual void force_stop();

	virtual void reset();

	virtual void reset_timer();

	virtual void logic() {}

	virtual void logic_reset() {}

	void logic_aggresive();

	void variant_initialize() override {};

	virtual void to_json_derived(json &) const {};

	virtual void from_json_derived(const json &j) {}

	virtual void serialize_publish_derived(
		const SerializeMap &map) {}

	virtual void serialize_initialize_derived(
		const SerializeMap &map) {}

	json to_json() const override;

	void from_json(const json &j) override;

	void serialize_publish(const SerializeMap &map) override;

	void serialize_initialize(const SerializeMap &map) override;
};

struct EntityEnemy : public EntityBody
{
	EnemyType enemyType = EnemyType::NONE;
	t_entities_itr<VariantPtr<EntityEnemy>> objItrEnemy;

	EntityEnemy();

	EntityEnemy(
		GameData *,
		const sf::Vector2f &pos,
		t_sprite spriteHolder,
		size_t id);
	
	bool confirm_body_derived(GameData* context) override;

	void update(float delta) override;

	void logic_reset() override;

	void logic() override;

	// Json stuff

	void to_json_derived(json &) const override;

	void from_json_derived(const json &j) override;

	void serialize_publish_derived(const SerializeMap &map) override;

	void serialize_initialize_derived(const SerializeMap &map) override;
};

struct EntityCitizen : public EntityBody
{
	CitizenJob job = CitizenJob::NONE;

	VariantPtr<BuildingBase> home = nullptr;
	VariantPtr<BuildingBase> workplace = nullptr;
	bool insideWorkplace = false;
	Resources rInventory, rInventoryCap;

	// When sending an entity to a location,
	// let him know what to do when he gets there
	Action nextAction = Action::NONE;

	// When the entity got to a buildings with 
	// resources to collect / gather, know what resources to move
	Resources rActionBool = { 0 };

	EntityCitizen();

	EntityCitizen(
		GameData *context,
		const sf::Vector2f &pos,
		t_sprite spriteHolder,
		size_t id);
	
	bool confirm_body_derived(GameData* context) override;

	void apply_data_derived(GameBodyConfig* config) override;

	// Json stuff

	void to_json_derived(json &) const override;

	void from_json_derived(const json &j) override;

	void serialize_publish_derived(const SerializeMap &map) override;

	void serialize_initialize_derived(const SerializeMap &map) override;

	// Logic

	void update(float delta) override;

	bool inventory_full(float precentageLimit = 0.8f) const;

	void logic() override;

	void logic_worker();

	void logic_test01();

	void logic_reset_worker();

	void logic_reset() override;

	PathData generate_path_worker(const IVec& end);

	void recalculate_worker_stats();
	
};

#endif // _GAME_ENTITY
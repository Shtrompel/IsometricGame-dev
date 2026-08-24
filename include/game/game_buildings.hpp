#ifndef _GAME_BUILDINGS
#define _GAME_BUILDINGS

#include "game/game_body.hpp"
#include "game/game_entity.hpp"
#include "game/power_network.hpp"
#include "utils/buildin_enums.hpp"

struct GameData;

// Building upgrade tree

struct TextureFrames
{
	IVec start = NULL_IVEC;
	IVec end = NULL_IVEC;
};

struct UpgradeTree : GameBodyConfig
{
	int id = NULL_INT;
	t_idpair type = { 0, 0 };
	char name[MAX_SHORT_STR] = NULL_STR;
	char description[256] = NULL_STR;
	int hp = NULL_INT;
	sf::Vector2i size = {1, 1};
	int buildWork = 0;
	t_alignment alignment = NULL_INT;

	char image[MAX_SHORT_STR] = NULL_STR;
	// Two int vectors containing the range of frames
	// from the texture sprite
	IVec framesStart = NULL_IVEC,
		framesEnd = NULL_IVEC;

	IVec framesUStart = NULL_IVEC,
		framesUEnd = NULL_IVEC;

	//t_sprite spriteHolder = -1;
	std::unordered_map<sf::Vector2i, t_sprite>
		spriteHolders = {{sf::Vector2i{0, 0}, -1}},
		spriteUHolders = { {sf::Vector2i{0, 0}, -1} };
	t_sprite spriteIcon = 0;

	char spriteImage[MAX_SHORT_STR] = NULL_STR;
	// TextureFrames spriteFrames{};
	// TextureFrames spriteFramesCollect{};
	// TextureFrames spriteFramesTransfer{};
	std::unordered_map<sf::Vector2i, t_sprite>
		spriteEntityHolders =
	{ {sf::Vector2i{0, 0}, -1} };

	Resources
		build = NULL_RES,
		input = NULL_RES,
		output = NULL_RES;
	int powerIn = 0,
		powerOut = 0,
		powerStore = 0;
	int weightCap = NULL_INT;
	Resources storageCap = NULL_RES;

	float effectRadius = NULL_FLOAT;
	int entityLimit = NULL_INT;

	t_seconds delayCost = NULL_FLOAT;
	t_seconds delayAction = NULL_FLOAT;

	CitizenJob job = CitizenJob::NONE;
	PropertySet<PropertyBool, PropertyNum> props;
	SpawnInfo spawn;

	// The id of the specific upgrade in the tree,
	// zero means base build
	int upgradeId = 0;
	// The id of the parent upgrade
	// equals to 0 if the parent if the
	// base building
	int upgradeParent = -1;
	// The id of the path from the father
	// equals to 0 if  this is the single
	// branching path from it's parent
	int upgradePath = -1;

	std::vector<UpgradeTree> children;
	UpgradeTree *parent = nullptr;

	t_sprite get_sprite(
		const sf::Vector2i &pos = {0, 0}, 
		bool unfunctional = false) const;

	std::string to_string(
		GameData *context = nullptr,
		bool verbose = true) const;
};

// "Readonly" variables
struct BuildingBaseInfo
{
	size_t id = 0;
	IVec tilePos = {0, 0};
	IVec tilesSize = {1, 1};

	char name[MAX_SHORT_STR] = "";

	t_id buildType = (t_id)BuildingType::EMPTY;
	size_t job = 0;
	size_t entitiesSpawned = 0u;
	SpawnInfo spawn;
	t_alignment alignment = ALIGNMENT_FRIENDLY;

	float effectRadius = 0.0f;
	int weightCap = -1;
	unsigned entityLimit = 0u;
	bool active = true, sufficient = true;

	Resources rIn, rOut;
	Resources rStoreCap;
	int powerIn = 0, powerOut = 0, powerStore = 0;

	PropertySet<PropertyBool, PropertyNum> props;

	UpgradeTree *tree = nullptr;
	std::unordered_map<IVec, t_sprite>
		sprites = { {IVec{0, 0}, -1} };
};

// "Dynamic/Volatile" variables
struct BuildingBaseData
{
	// Minus one for infinite health / untargetable
	int hp = -1;
	int lastHp = -1;

	bool operational = true;
	bool flagDelete = false;
	// Workers / inhabitants
	std::vector<VariantPtr<EntityBody>> entities;
	// Entities that work inside,
	// useful only if "areEntitiesInside" is true
	std::vector<VariantPtr<EntityBody>> storedEntities;
	// Generates/Takes resources every fixed time
	t_body_timer costTimer;
	t_body_timer actionTimer;

	// Animation frame
	int animFrame = -1;

	Resources rStorage;
	Resources rPending;

	int powerValue = 0;
	int lastPowerOut = 0;
	int finalPowerOut = 0;

	std::vector<VariantPtr<BuildingBase>> binded;
	VariantPtr<PowerNetwork> network = nullptr;
	bool updateInfo = false;
	std::vector<VariantPtr<GameBody>> followers;



	// ID of every upgrade
	std::vector<int> upgrades;
};

// Todo inheritance to composition
struct BuildingBase : BuildingBaseInfo,
					  BuildingBaseData,
					  public Variant
{
	GameData *context = nullptr;
	BuildingBaseInfo *info = dynamic_cast<BuildingBaseInfo *>(this);
	BuildingBaseData *data = dynamic_cast<BuildingBaseData *>(this);

	BuildingBase();

	BuildingBase(
		GameData *context,
		const sf::Vector2i &pos,
		UpgradeTree *step,
		const t_build_enum &type,
		size_t id);

	virtual ~BuildingBase() {}
	
	UpgradeTree* get_tree(t_id = 0ul);

	/**
	 * When type "Variant" is initialized,
	 * Initialize 
	 */
	void variant_initialize() override;

	json to_json() const override;

	void from_json(const json &j) override;

	void serialize_publish(const SerializeMap &map) override;

	void serialize_initialize(const SerializeMap &map) override;

	std::string to_string();

	static std::string get_format_str(const std::string&);

	std::string get_format_name();

	FVec get_center_pos();

	// Upgrades

	void load_upgrade_step(UpgradeTree *step, void *tree = 0);

	std::vector<UpgradeTree *> get_upgrades();

	void update_sprites(int rotation);

	bool tree_similar(BuildingBase *);

	// Gameplay

	inline void damage(int attack);

	// BuildingBody

	BuildingBody *get_body() const;

	std::list<BuildingBody *> get_bodies();

	// Logic

	bool is_any_storage() const;

	inline bool is_operational() const;

	inline void flag_deletion();

	void update();

	// "Hire" citizen to a worker
	void accept_entity(EntityCitizen *);

	// Give the worker their job 
	void accept_entity_job(EntityCitizen*);

	// When a worker comes inside
	void enter_entity(EntityCitizen *);

	// When a worker is going outside
	void exit_entity(EntityCitizen *);

	// "Fire" the worker back to a jobless citizen
	std::vector<VariantPtr<EntityBody>>::iterator remove_entity(EntityCitizen *);
};

struct BuildingBody
	: public GameBody
{
	sf::Vector2i tilePos;

	sf::Vector2i start = {-1, -1};
	sf::Vector2i end = {-1, -1};
	t_sprite spriteHolder = -1;
	int frame = 0u;
	int maxFrame = 0u;
	sf::Vector2i spriteIndex = {0, 0};

	VariantPtr<BuildingBase> base{nullptr};

	BuildingBody() {}

	BuildingBody(
		GameData *context,
		const sf::Vector2i &pos,
		BuildingBase *base,
		t_sprite spriteHolder,
		sf::Vector2i spriteIndex = {0, 0},
		size_t id = 0);

	virtual ~BuildingBody();

	int get_hp() const override;

	int& get_hp() override;

	void apply_data(GameBodyConfig* config) override;
	
	bool confirm_body(GameData* context) override;
	
	t_sprite get_sprite(int rotation) const;

	void variant_initialize() override {}

	json to_json() const override;

	void from_json(const json &j) override;

	void serialize_publish(const SerializeMap &map) override;

	void serialize_initialize(const SerializeMap &map) override;
};

#endif // _GAME_BUILDINGS
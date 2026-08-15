#ifndef _GAME_GRID
#define _GAME_GRID

#include "../file/serialization.hpp"
#include "../utils/container/quad_tree.hpp"

#include <mutex>

struct EntityBody;
struct BuildingBody;
struct BuildingBase;
struct EntityCitizen;
struct EntityEnemy;

// Single 1x1 tile
struct Tile
{
	// Tile properties
	bool visible = true;
	bool active = true;
	t_byte speedBonus = 0u;
	t_byte attackBonud = 0u;
	t_byte bodyBonus = 0u;
	
	sf::Vector2i tilePos;

	bool _selected = false;

	// GameBody
	 VariantPtr<BuildingBody> building = nullptr;
	 std::list<VariantPtr<EntityBody>> entities;

	bool is_barrier() const;

	bool remove_entity(EntityBody *e);

	BuildingBase* get_building();

	// void serialize_post(const SerializeMap &map);
	
};

// 16x16 chunk
struct Grid
{
	mutable std::mutex gridMutex;

	Tile *grid = nullptr;
	// Tile count and position
	int cx = 0, cy = 0;
	int idx, idy;
	
	template <class T>
	using t_set = std::unordered_set<T>;
	
	template <class T>
	using t_ptr = VariantPtr<T>;

	t_set<t_ptr<BuildingBody>> setBuildings;
	t_set<t_ptr<EntityCitizen>> setCitizens;
	t_set<t_ptr<EntityEnemy>> setEnemies;

	Grid();

	Grid(int cx, int cy, int idx, int idy);

	~Grid();

	Tile &operator()(int x, int y) const;

	void remove_entity(EntityBody*entity);

	void insert_entity(EntityBody *entity);

	void insert_building(BuildingBody *build);

	void remove_building(BuildingBody *build);
};

//
struct Chunks : Variant
{
	mutable std::recursive_mutex chunksMutex;

	std::unordered_map<unsigned, Grid *> available;
	size_t gridw, gridh;

	static unsigned gen_key(short x, short y);
	
	Chunks() : gridw(0), gridh(0) {}

	Chunks(size_t gridw, size_t gridh);

	~Chunks();

	void clear();
	
	json to_json() const override;

	void from_json(const json &j) override;

	void serialize_publish(const SerializeMap &map) override;
	
	void serialize_initialize(const SerializeMap &map) override;
	
	void add(Grid *grid);

	bool has_tile(int x, int y) const;

	Grid *get(short indexX, short indexY) const;

	Grid *get_grid(int posX, int posY) const;

	Tile &get_tile(int x, int y) const;

	bool is_free(int x, int y) const;

	// Combines has_tile and get_tile, Thread safe
	Tile const *get_tile_safe(int x, int y) const;

	std::pair<Grid *, Tile &> get_pair(int x, int y) const;

	bool has_build(int x, int y) const;

	BuildingBody *get_build(int x, int y) const;
};

#endif // _GAME_GRID
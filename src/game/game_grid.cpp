
#include "game/game_grid.hpp"

#include "game/game_entity.hpp"
#include "game/game_buildings.hpp"

// Tile

bool Tile::is_barrier() const
{
	return (building &&
			building->base->props.bool_is(PropertyBool::BARRIER));
}

bool Tile::remove_entity(EntityBody *e)
{
	auto it = std::find(entities.begin(), entities.end(), e);
	if (it != entities.end())
	{
		entities.erase(it);
		return true;
	}
	return false;
}

BuildingBase *Tile::get_building()
{
	if (building && building->base)
		return building->base;
	return nullptr;
}

static void serialize_post(const SerializeMap &map, Tile &tile)
{
	for (auto &v : tile.entities)
	{
		// DEBUG("%s", v.to_json_verbose().dump().c_str());
		map.apply(v);
	}
	if (tile.building.is_valid())
	{
		// DEBUG("%s", tile.building.to_json_verbose().dump().c_str());
		map.apply(tile.building);
	}
}

inline uint64_t code_write(
	size_t &codeFollower,
	uint64_t code,
	bool data)
{
	code ^= (uint64_t)data << codeFollower;
	codeFollower += 1;
	return code;
}

inline uint64_t code_write(
	size_t &codeFollower,
	uint64_t code,
	uint8_t data)
{
	code ^= (uint64_t)data << codeFollower;
	codeFollower += 8;
	return code;
}

template <typename T>
inline T code_read(
	size_t &codeFollower,
	uint64_t code,
	size_t size = sizeof(T))
{
	code = code >> codeFollower;
	code &= ((1 << size) - 1);
	return static_cast<T>(code);
}

inline bool code_read_bool(
	size_t &follower,
	uint64_t code)
{
	return code_read<bool>(follower, code, 1);
}

inline bool code_read_uint8(
	size_t &follower,
	uint64_t code)
{
	return code_read<uint8_t>(follower, code);
}

static void to_json(json &j, const Tile &v)
{
	const Tile *t = &v;

	uint64_t code = 0u;
	size_t follower = 0;
	j[0] = t->tilePos;
	code = code_write(follower, code, t->visible);
	code = code_write(follower, code, t->active);
	code = code_write(follower, code, t->speedBonus);
	code = code_write(follower, code, t->attackBonud);
	code = code_write(follower, code, t->bodyBonus);
	j[1] = code;
#if defined(DEEP_SERIALIZE_GRID)
	if (t->building)
		j[2] = t->building.to_json_ptr();
	if (t->entities.size())
		j[3] = t->entities;
#endif // defined(DEEP_SERIALIZE_GRID)
}

static void from_json(const json &j, Tile &v)
{
	try
	{
		Tile *t = &v;
		uint64_t code = 0u;
		size_t follower = 0;
		j.at(0).get_to(t->tilePos);
		j.at(1).get_to(code);
		t->visible = code_read_bool(follower, code);
		t->active = code_read_bool(follower, code);
		t->speedBonus = code_read_uint8(follower, code);
		t->attackBonud = code_read_uint8(follower, code);
		t->bodyBonus = code_read_uint8(follower, code);

#if defined(DEEP_SERIALIZE_GRID)
		if (j.size() > 2 && !j.at(2).is_null())
			j.at(2).get_to(t->building);
		if (j.size() > 3 && !j.at(3).is_null())
			j.at(3).get_to(t->entities);
#else
		t->building = nullptr;
		t->entities.clear();
#endif // defined(DEEP_SERIALIZE_GRID)
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize Tile %s", e.what());
	}
}

// Grid

Grid::Grid()
{
	grid = nullptr;
}

Grid::Grid(int cx, int cy, int idx, int idy)
	: cx(cx), cy(cy), idx(idx), idy(idy)
{
	if (cx != 0 && cy != 0)
	{
		grid = new Tile[cx * cy];
		for (int i = 0; i < cx * cy; ++i)
		{
			auto &p = grid[i].tilePos;
			p.x = i % cx + cx * idx;
			p.y = i / cx + cy * idy;
		}
	}
}

Grid::~Grid()
{
	delete[] grid;
}
/*
static void to_json(json &j, const Grid &vr)
{
const Grid *v = &vr;
	j["c"] = {v->cx, v->cy};     // Primitives are safe
	j["id"] = {v->idx, v->idy};  // Primitives are safe

	if (v->setBuildings.size() ||
		v->setCitizens.size() ||
		v->setEnemies.size())
	{
		// FIX: Explicit array construction for complex sets
		json setsArr = json::array();
		setsArr.push_back(v->setBuildings);
		setsArr.push_back(v->setCitizens);
		setsArr.push_back(v->setEnemies);
		j["sets"] = setsArr;
	}

	Tile *t = v->grid;
	for (int i = 0; i < v->cx * v->cy; ++i, ++t)
	{
		if (t) j["t"][i] = *t;
	}
}
*/
static void to_json(json &j, const Grid &vr)
{
	try
	{
		const Grid *v = &vr;

		json cArr = json::array();
		cArr.push_back(v->cx);
		cArr.push_back(v->cy);
		j["c"] = cArr;

		json idArr = json::array();
		idArr.push_back(v->idx);
		idArr.push_back(v->idy);
		j["id"] = idArr;

#if defined(DEEP_SERIALIZE_GRID)
		if (v->setBuildings.size() ||
			v->setCitizens.size() ||
			v->setEnemies.size())
		{
			// FIX: Explicit array construction for complex sets
			json setsArr = json::array();
			setsArr.push_back(v->setBuildings);
			setsArr.push_back(v->setCitizens);
			setsArr.push_back(v->setEnemies);
			j["sets"] = setsArr;
		}
#endif // defined(DEEP_SERIALIZE_GRID)

/*
		Tile *t = v->grid;
		for (int i = 0; i < v->cx * v->cy; ++i, ++t)
		{
			if (t)
				j["t"][i] = *t;
		}
*/

		// Seriaize JSON code as list of chunks
		size_t gridCount = v->cx * v->cy;
		std::vector<uint64_t> tileCodes(gridCount, 0);
		
		Tile *t = v->grid;
		for (size_t i = 0; i < gridCount; ++i, ++t)
		{
			Tile& t = v->grid[i];

			uint64_t code = 0u;
			size_t follower = 0;
			code = code_write(follower, code, t.visible);
			code = code_write(follower, code, t.active);
			code = code_write(follower, code, t.speedBonus);
			code = code_write(follower, code, t.attackBonud);
			code = code_write(follower, code, t.bodyBonus);
			tileCodes[i] = code;
		}
		
		j["t_codes"] = tileCodes;
	}
	catch (const std::exception &e)
	{
		LOG_ERROR("Unable to convert to json Grid: %s", e.what());
	}
}

static void from_json(const json &j, Grid &v)
{
	try
	{
		j.at("c").at(0).get_to(v.cx);
		j.at("c").at(1).get_to(v.cy);
		j.at("id").at(0).get_to(v.idx);
		j.at("id").at(1).get_to(v.idy);
		// DEBUG("%d %d, %d %d",v.cx, v.cy, v.idx, v.idy );

#if defined(DEEP_SERIALIZE_GRID)
		if (j.count("sets"))
		{
			j.at("sets").at(0).get_to(v.setBuildings);
			j.at("sets").at(1).get_to(v.setCitizens);
			j.at("sets").at(2).get_to(v.setEnemies);
		}
#else
		v.setBuildings.clear();
		v.setCitizens.clear();
		v.setEnemies.clear();
#endif // defined(DEEP_SERIALIZE_GRID)

		/*
		size_t gridCount = v.cx * v.cy;
		v.grid = new Tile[gridCount];
		for (size_t i = 0; i < gridCount; ++i)
		{
			j.at("t").at(i).get_to(v.grid[i]);
		}
		*/

		size_t gridCount = v.cx * v.cy;
		v.grid = new Tile[gridCount];

		if (j.contains("t_codes"))
		{
			std::vector<uint64_t> tileCodes;
			j.at("t_codes").get_to(tileCodes);

			for (size_t i = 0; i < gridCount; ++i)
			{
				Tile &t = v.grid[i];
				uint64_t code = tileCodes[i];
				size_t follower = 0;

				t.visible = code_read_bool(follower, code);
				t.active = code_read_bool(follower, code);
				t.speedBonus = code_read_uint8(follower, code);
				t.attackBonud = code_read_uint8(follower, code);
				t.bodyBonus = code_read_uint8(follower, code);

				// Mathematically reconstruct the sf::Vector2i positions
				t.tilePos.x = i % v.cx + v.cx * v.idx;
				t.tilePos.y = i / v.cx + v.cy * v.idy;
				t.building = nullptr;
				t.entities.clear();
			}
		}
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize Grid %s", e.what());
	}
}

Tile &Grid::operator()(int x, int y) const
{
	// std::lock_guard<std::mutex> glock(gridMutex);
	assert(grid);
	assert(0 <= x && x < cx && 0 <= y && y < cy);
	return grid[y * cx + x];
}

void Grid::remove_entity(EntityBody *entity)
{
	// std::lock_guard<std::mutex> glock(gridMutex);
	assert(entity);
	if (entity->entityType == EntityType::CITIZEN)
		setCitizens.erase(dynamic_cast<EntityCitizen *>(entity));
	else if (entity->entityType == EntityType::ENEMY)
		setEnemies.erase(dynamic_cast<EntityEnemy *>(entity));
}

void Grid::insert_entity(EntityBody *entity)
{
	// std::lock_guard<std::mutex> glock(gridMutex);
	assert(entity);
	if (entity->entityType == EntityType::CITIZEN)
		setCitizens.insert(dynamic_cast<EntityCitizen *>(entity));
	else if (entity->entityType == EntityType::ENEMY)
		setEnemies.insert(dynamic_cast<EntityEnemy *>(entity));
}

void Grid::insert_building(BuildingBody *build)
{
	// std::lock_guard<std::mutex> glock(gridMutex);
	assert(build);
	setBuildings.insert(build);
}

void Grid::remove_building(BuildingBody *build)
{
	// std::lock_guard<std::mutex> glock(gridMutex);
	assert(build);
	setBuildings.erase(build);
}

// Chunks

Chunks::Chunks(size_t gridw, size_t gridh)
	: Variant(SERIALIZABLE_CHUNKS, 0),
	  gridw(gridw), gridh(gridh)
{
}

Chunks::~Chunks()
{
	clear();
}

void Chunks::clear()
{
	for (auto itr = available.begin(); itr != available.end(); itr++)
	{
		delete (*itr).second;
	}
	available.clear();
}

json Chunks::to_json() const
{
	json j;

	json gArr = json::array();
	gArr.push_back(gridw);
	gArr.push_back(gridh);
	j["g"] = gArr;

	for (auto &x : available)
	{
		j["a"][std::to_string(x.first)] = *x.second;
	}
	return j;
}

void Chunks::from_json(const json &j)
{
	try
	{
		j.at("g").at(0).get_to(gridw);
		j.at("g").at(1).get_to(gridh);
		size_t i = 0;
		for (auto itr = j.at("a").begin();
			 itr != j.at("a").end();
			 ++itr, ++i)
		{
			const json &jsonGrid = itr.value();
			int k = (unsigned)std::stol(itr.key());
			Grid *grid = new Grid{};
			available.insert({k, grid});
			jsonGrid.get_to(*grid);
		}
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize BuildingBody: %s", e.what());
	}
}

void Chunks::serialize_publish(const SerializeMap &map)
{
#if defined(DEEP_SERIALIZE_GRID)
	for (volatile auto &p : available)
	{
		auto &grid = *p.second;

		Tile *buffFollower = grid.grid;
		for (int i = 0; i < grid.cx * grid.cy; ++i, ++buffFollower)
		{
			assert(buffFollower);
			::serialize_post(map, *buffFollower);
		}

		// Unsafe iteration over keys of a hash set
		// It's okay because the hash value stayes the same
		for (auto &v : grid.setBuildings)
		{
			auto &b = const_cast<
				Grid::t_ptr<BuildingBody> &>(v);
			map.apply(b);
		}
		for (auto &v : grid.setCitizens)
		{
			auto &b = const_cast<
				Grid::t_ptr<EntityCitizen> &>(v);
			map.apply(b);
		}
		for (auto &v : grid.setEnemies)
		{
			auto &b = const_cast<
				Grid::t_ptr<EntityEnemy> &>(v);
			map.apply(b);
		}
	}
#endif // defined(DEEP_SERIALIZE_GRID)
}

void Chunks::serialize_initialize(const SerializeMap &map)
{
}

unsigned Chunks::gen_key(short x, short y)
{
	unsigned v = (unsigned)x + ((unsigned)y << 16);
	return v;
}

void Chunks::add(Grid *grid)
{
	// std::lock_guard<std::recursive_mutex> lock(chunksMutex);

	unsigned v = gen_key(grid->idx, grid->idy);
	available[v] = grid;
}

bool Chunks::has_tile(int x, int y) const
{
	short idx = (short)math_floordiv(x, (int)gridw);
	short idy = (short)math_floordiv(y, (int)gridh);
	unsigned v = gen_key(idx, idy);

	// std::lock_guard<std::recursive_mutex> lock(chunksMutex);
	if (!available.count(v))
		return false;

	if (!get_tile(x, y).active)
		return false;

	return true;
}

Grid *Chunks::get(short indexX, short indexY) const
{
	// std::lock_guard<std::recursive_mutex> olock(chunksMutex);

	unsigned v = gen_key(indexX, indexY);
	if (!available.count(v))
	{
		return nullptr;
	}
	return available.at(v);
}

Grid *Chunks::get_grid(int posX, int posY) const
{
	short idx = (short)math_floordiv(posX, (int)gridw);
	short idy = (short)math_floordiv(posY, (int)gridh);

	// std::lock_guard<std::recursive_mutex> olock(chunksMutex);

	Grid *g = get(idx, idy);
	return g;
}

Tile &Chunks::get_tile(int x, int y) const
{
	// std::lock_guard<std::recursive_mutex> olock(chunksMutex);
	return get_pair(x, y).second;
}

bool Chunks::is_free(int x, int y) const
{
	return !has_build(x, y) && has_tile(x, y);
}

Tile const *Chunks::get_tile_safe(int x, int y) const
{
	// std::lock_guard<std::recursive_mutex> glock(chunksMutex);
	if (!has_tile(x, y))
		return nullptr;
	return &get_pair(x, y).second;
}

std::pair<Grid *, Tile &> Chunks::get_pair(int x, int y) const
{
	short idx = (short)math_floordiv(x, (int)gridw);
	short idy = (short)math_floordiv(y, (int)gridh);

	// std::lock_guard<std::recursive_mutex> olock(chunksMutex);
	Grid *grid = get(idx, idy);
	Tile &tile = (*grid)(
		math_mod(x, (int)gridw),
		math_mod(y, (int)gridh));
	tile.tilePos = {x, y};
	return std::pair<Grid *, Tile &>{grid, tile};
}

bool Chunks::has_build(int x, int y) const
{
	// std::lock_guard<std::recursive_mutex> olock(chunksMutex);
	if (!has_tile(x, y))
		return false;
	auto ret = get_tile(x, y).building != nullptr;
	return ret;
}

BuildingBody *Chunks::get_build(int x, int y) const
{
	// std::lock_guard<std::recursive_mutex> glock(chunksMutex);
	if (!has_build(x, y))
		return nullptr;
	return get_tile(x, y).building;
}
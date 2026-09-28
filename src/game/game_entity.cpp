#include "game/game_entity.hpp"

#include <cassert>
#include <string>
#include <vector>

#include "utils/class/logger.hpp"
#include "utils/class/resources.hpp"
#include "utils/math.hpp"
#include "utils/utils.hpp"
#include "utils/globals.hpp"

#include "game/game_buildings.hpp"
#include "game/game_grid.hpp"
#include "game/game_data.hpp"

#include "utils/pathfind.hpp"

#include <iterator>

extern "C"
{
#include "libs/prng.h"
};

#define EPIC_TEST2()

#define TRANSFER_TB_DEBUG(entity, build)                                                         \
	DEBUG("\n%s: %s %s %d %s\nResult: %d, %d",                                                   \
		  build->name,                                                                           \
		  CSTR(entity->rInventory),                                                              \
		  CSTR(build->rStorage),                                                                 \
		  build->weightCap,                                                                      \
		  CSTR(build->rStoreCap),                                                                \
		  Resources::can_transfer_weight(entity->rInventory, build->rStorage, build->weightCap), \
		  Resources::can_transfer_limit(entity->rInventory, build->rStorage, build->rStoreCap));

PathData::PathData(size_t id)
	: Variant(SERIALIZABLE_PATH, id)
{
}

bool PathData::valid() const
{
	return destination &&
		   path.size() &&
		   path.back().value == destination->tilePos;
}

inline bool PathData::finished() const
{
	return follower == path.cend();
}

std::string PathData::to_string() const
{
	std::stringstream str("");
	str << "{";
	for (auto &v : path)
		str << "{ " << v.index << " : " << vec_str(v.value) << "}, ";
	str << "}";
	return str.str();
}

// https://devblogs.microsoft.com/oldnewthing/20221114-00/?p=107393
constexpr std::size_t constexpr_strlen(const char *s)
{
	return std::char_traits<char>::length(s);
	// or
	return std::string::traits_type::length(s);
}

std::string PathData::why_invalid() const
{
	if (!destination)
		return std::string("No destination");
	else if (path.empty())
		return std::string("No path");
	else if (path.back().value != destination->tilePos)
	{
		constexpr const char ogMsg[] =
			"Path doesn't lead to destination: Path stops in %s while dest is %s";
		char msg[constexpr_strlen(ogMsg) + 16];
		sprintf(msg,
				ogMsg,
				VEC_CSTR(path.back().value),
				VEC_CSTR(destination->tilePos));
		return std::string(msg);
	}

	return std::string("Valid");
}

sf::Vector2i PathData::front() const
{
	return (*follower).value;
}

void PathData::pop()
{
	assert(follower != path.cend());
	follower++;
}

void PathData::clear()
{
	destination = nullptr;
	path.clear();
}

[[deprecated]] void PathData::append(const PathData &in)
{
	int i = 0;
	if (path.size())
		i = path.back().index;

	ptrdiff_t x = std::distance(path.cbegin(), follower);

	if (path.size())
	{
		path.pop_back();
		x -= 1;
	}

	for (const IndexVec &indxVec : in.path)
	{
		if (path.back().value == indxVec.value)
			continue;

		IndexVec toAdd = IndexVec{indxVec};
		toAdd.index = ++i;
		this->path.push_back(toAdd);
	}

	this->follower = std::next(path.cbegin(), x);
}

static void to_json(json &j, const PathData::IndexVec &v)
{
	try
	{
		json arr = json::array();
		arr.push_back(v.index);
		arr.push_back(v.value);
		j = arr;
	}
	catch (const std::exception &e)
	{
		LOG_ERROR("Unable to convert to json PathData::IndexVec: %s", e.what());
	}
}

static void from_json(const json &j, PathData::IndexVec &v)
{
	try
	{
		j.at(0).get_to(v.index);
		j.at(1).get_to(v.value);
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize PathData::IndexVec %s", e.what());
	}
}

json PathData::to_json() const
{
	json j;
	size_t i = 0;
	for (auto itr = path.begin(); itr != path.end(); ++itr, ++i)
		j["path"][i] = *itr;
	j["pos"] = this->destPos;
	if (path.size())
		j["index"] = (size_t)std::distance(path.begin(), follower);
	return j;
}

void PathData::from_json(const json &j)
{
	try
	{
		if (j.count("path"))
			j.at("path").get_to(this->path);
		if (j.count("pos"))
			j.at("pos").get_to(this->destPos);
		if (j.count("index"))
		{
			size_t i = j.at("index").get<size_t>();
			this->follower = std::next(path.begin(), i);
		}
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize PathData: %s", e.what());
	}
}

void PathData::serialize_publish(const SerializeMap &map)
{
}

void PathData::serialize_initialize(const SerializeMap &map)
{
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);
	// this->objectId = g->counterContext.objectIdCounter.advance<PathData>();
	this->destination =
		&g->chunks->get_tile(destPos.x, destPos.y);
}

// Json stuff

void to_json(json &j, const PathData &v)
{
	j = v.to_json();
}

void from_json(const json &j, PathData &v)
{
	v.from_json(j);
}

void to_json(json &j, const t_idpair &v)
{
	json arr = json::array();
	arr.push_back(v.group);
	arr.push_back(v.id);
	j = arr;
}

void from_json(const json &j, t_idpair &v)
{
	try
	{
		j.at(0).get_to(v.group);
		j.at(1).get_to(v.id);
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize t_idpair %s", e.what());
	}
}

void to_json(json &j, const SpawnInfo &v)
{
	try
	{
		json arr = json::array();
		arr.push_back(v.id);
		arr.push_back(v.serializableID);
		j = arr;
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Unable to convert to json SpawnInfo: %s", e.what());
	}
}

void from_json(const json &j, SpawnInfo &v)
{
	try
	{
		j.at(0).get_to(v.id);
		j.at(1).get_to(v.serializableID);
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize SpawnInfo %s", e.what());
	}
}

// EntityStats

nlohmann::json EntityStats::to_json() const
{
	nlohmann::json ret;

	ret["id"] = id;
	ret["type"] = type;
	ret["hp"] = hp;
	ret["alignment"] = alignment;
	ret["props"] = props;
	ret["inventorySize"] = inventorySize;
	ret["transferSize"] = transferSize;
	ret["maxSpeed"] = maxSpeed;
	ret["maxForce"] = maxForce;
	ret["spriteWalk"] = spriteWalk;
	ret["spriteAction"] = spriteAction;
	ret["spriteHit"] = spriteHit;
	ret["caction"] = cooldownAction;
	ret["cpath"] = cooldownPath;

	return ret;
}

std::string EntityStats::to_string()
{
	return to_json().dump(1).c_str();
}

// Entity

EntityBody::EntityBody(
	GameData *context,
	const sf::Vector2f &pos,
	t_sprite sprite,
	size_t e,
	size_t id)
	: GameBody(
		  context,
		  e,
		  id,
		  pos,
		  sprite,
		  BodyType::ENTITY,
		  ENTITY_COLLISION_SIZE)
{
	this->timerAction =
		t_time_manager::make_independent_timer();
	this->timerPath =
		t_time_manager::make_independent_timer();

	this->shape = ShapeType::CIRCLE;
	// todo remove (default is 0)
	this->radius = 0.15f * 0.8f;

	localClock = t_time_manager::make_independent_timer();
}

void EntityBody::apply_data(GameBodyConfig *config)
{

	if (static_cast<EntityStats *>(config) == nullptr)
	{
		WARNING(
			"In function %s invalid type of input function parameter, should be of type GameBodyConfig",
			__PRETTY_FUNCTION__);
	}

	auto &stats = *static_cast<EntityStats *>(config);

	this->hp = stats.hp;

	this->radius = radius;
	this->alignment = stats.alignment;
	this->inventorySize = stats.inventorySize;
	this->transferSize = stats.transferSize;
	this->maxSpeed = stats.maxSpeed;
	this->maxForce = stats.maxForce;
	this->aggressive = stats.aggressive;
	this->isLightSource = stats.isLightSource;
	this->timerAction->set_length(stats.cooldownAction);
	this->timerPath->set_length(stats.cooldownPath);
	this->props = stats.props;
	this->spriteWalk = stats.spriteWalk;
	this->spriteAction = stats.spriteAction;
	this->spriteHit = stats.spriteHit;
	this->spriteHolder = this->spriteWalk;

	if (stats.spawnID != 0 && stats.spawnType != IDPAIR_NONE)
		this->spawn = {stats.spawnID, stats.spawnType};

	apply_data_derived(config);
}

void EntityBody::apply_data_derived(GameBodyConfig *config)
{
}

bool EntityBody::confirm_body(GameData *context)
{

	// this->objectId = context->counterContext.objectIdCounter.advance<EntityBody>();

	if (!GameBody::confirm_body(context))
		return false;
	sf::Vector2i ipos = vec_pos_to_tile(this->pos);

	this->context = context;
	context->bodiesContext.bodies.push_back(this);

	if (this->visible) 
	{
		Grid *grid = context->chunks->get_grid(ipos.x, ipos.y);
		assert(grid);

		grid->insert_entity(dynamic_cast<EntityBody *>(this));
		if (!context->chunks->has_tile(ipos.x, ipos.y))
			return false;

		Tile &tile = context->chunks->get_tile(ipos.x, ipos.y);
		for (auto &pair : context->get_trees(this))
		{
			auto tree = pair.second;
			ASSERT_ERROR(
				(tree->insert(ipos, dynamic_cast<GameBody *>(this))),
				"QuadTree Citizen can't insert citizen.");
		}

		tile.entities.push_back(this);
	}

	// For entity enemy and entity citizen
	if (!confirm_body_derived(context))
		return false;

	return true;
}

bool EntityBody::confirm_body_derived(GameData *context)
{
	return true;
}

#define AMONGUS()                              \
	if (this->alignment == ALIGNMENT_FRIENDLY) \
		AAAA;

bool EntityBody::search_entity()
{
	this->action = Action::IDLE;
	float weightBase, weightSub;
	int pathMax = 0;

	typedef EntityPropertyNums EP;
	typedef EntityPropertyBools EB;

	if (props.num_is(EP::PATH_WEIGHT_BASE))
		weightBase = (float)props.num_get(EP::PATH_WEIGHT_BASE);
	else
		weightBase = (float)context->get_const(
			ConstantFloating::PATH_WEIGHT_BASE);

	if (props.num_is(EP::PATH_WEIGHT_SUB))
		weightSub = (float)props.num_get(EP::PATH_WEIGHT_SUB);
	else
		weightSub = (float)context->get_const(
			ConstantFloating::PATH_WEIGHT_SUB);

	if (props.num_is(EP::PATH_COUNT))
		pathMax = (int)props.num_get(EP::PATH_COUNT);
	else
	{
		pathMax = (int)context->get_const(
			ConstantNumeric::PATH_COUNT);
	}

	/**
	 * When finding the nearest object to attack, don't just choose the
	 * nearest but also target weaker
	 */
	auto calculateWeight = [this, weightBase, weightSub](
							   const GameBody *g)
	{
		int hp = g->get_hp();
		float multiplier = 1, weight;

		if (g->type == BodyType::BUILDING)
		{
			BuildingBase *b;
			b = dynamic_cast<const BuildingBody *>(g)->base;
			// Buildings can use a propery that makes them more or less liekly to get hit
			if (b->props.num_is(PropertyNum::PATH_WEIGHT_MULT))
				multiplier *= (float)b->props.num_get(PropertyNum::PATH_WEIGHT_MULT);
		}

		// DEBUG("Compare 1: %s %s", g->get_ptr_str().c_str(), context->enum_get_str(g).c_str());
		// DEBUG("Compare 2: %d", hp);

		float weightHp = (float)g->get_hp();
		float arg = logf(weightHp) - weightSub;
		if (weightBase != 1 && weightBase > 0)
		{
			weight = (float)(arg);
			weight /= logf(weightBase);
		}

		float dist = vec_distsq(this->pos, g->pos);
		dist = dist + weight * multiplier;

		// Code made by Claude Sonnet 5 - berserk entities draw enemy attention
		if (g->type == BodyType::ENTITY)
		{
			const EntityBody *e = dynamic_cast<const EntityBody *>(g);
			if (e && e->isBerserk)
				dist *= BERSERK_TARGET_DISTANCE_SCALE;
		}
		return dist;
	};

	t_idpair targetType;
	{
		t_alignment side;
		side = context->alignment_opposite(this->alignment);

		bool a = !props.bool_is(EB::IGNORE_BUILDINGS);
		bool b = !props.bool_is(EB::IGNORE_ENTITIES);
		if (a && b)
			targetType = {ENUM_ALIGNMENT, (t_id)side};
		else if (a && !b)
			targetType = {ENUM_BODY_TYPE, (t_id)BodyType::BUILDING};
		else if (!a && b)
			targetType = {ENUM_BODY_TYPE, (t_id)BodyType::ENTITY};
		else
			targetType = {ENUM_BODY_TYPE, (t_id)BodyType::NONE};
	}

	if (!this->target)
	{
		/*
		 * Find nearest friendly objects to attack
		 * Try to avoid obkects with high health amount by
		 * giving them lower "priority" when sorting
		 * the best object to attack
		 */
		size_t thisAligment = alignment;

		auto entities = context->nearest_bodies_quad(
			this->pos,
			pathMax,
			targetType,
			[thisAligment](const sf::Vector2i &pos,
								 const GameBody *gb,
								 const int dist)
			{
				return !gb->dead &&
					   gb->get_hp() > 0 &&
					   gb->context->alignment_compare(
						   gb->alignment,
						   thisAligment) ==
						   ALIGNMENTS_ENEMIES;
			},
			[calculateWeight](
				const IVec &center,
				const IVec &,
				const GameBody *a,
				const IVec &,
				const GameBody *b)
			{
				float distA, distB;
				// distA = vec_distsq(this->pos, a->pos);
				// distB = vec_distsq(this->pos, b->pos);
				distA = calculateWeight(a);
				distB = calculateWeight(b);
				// Prefer a over b
				return distA < distB;
			});

		if (entities.empty())
			return false;

		assert(entities.front() != dynamic_cast<GameBody *>(this));

		// If found entity, set it as target
		this->set_target(entities.front());
	}

	assert(this->target);

	const int dijkstra = (int)context->get_const(
		t_constnum::A_STAR_DIJKSTRA_VALUE);
	const int greed = (int)context->get_const(
		t_constnum::A_STAR_GREED_VALUE);

	// std::vector<VariantPtr<GameBody>>* followers = nullptr;
	if (this->target->type == BodyType::ENTITY)
	{
		// EntityBody* e = dynamic_cast<EntityBody*>(this->target.get());
		// followers = &e->followers;
	}
	else if (this->target->type == BodyType::BUILDING)
	{
		// followers = &dynamic_cast<BuildingBody*>(this->target.get())->base->followers;
	}
	else
	{
		DEBUG("%d %s", (int)this->target->type, vec_str(this->target->pos).c_str());
		assert(0);
		return false;
	}

	targetLPos = vec_pos_to_tile(this->target->pos);
	auto path = generate_path(
		vec_pos_to_tile(pos),
		targetLPos,
		context->chunks,
		dijkstra,
		greed,
		-1.f,
		props.bool_is(EB::CAN_PHASE));

	go_to_building(path);

	return true;
}

void EntityBody::go_to_building(PathData &path)
{
	// Code made by Claude Sonnet 5 - generate_path() returns a destination-less
	// PathData ({}) when the start tile is a barrier or already equals the
	// target (e.g. an aggressive entity searching while standing on its
	// target's tile); guard the dereference instead of crashing.
	if (path.destination && path.destination->building)
	{
		BuildingBody* bb;
		bb = static_cast<BuildingBody*>(path.destination->building);
		this->set_target(bb);
	}
	
	set_path(std::move(path));
}

int EntityBody::get_hp() const
{
	return this->hp;
}

void EntityBody::set_hp(int value)
{
	this->hp = value;
}

double EntityBody::get_base(EntityPropertyNums n) const
{
	return props.num_get_or(n, 0.0);
}

double EntityBody::get_delta(EntityPropertyNums n) const
{
	auto itr = propsDelta.find(n);
	return itr == propsDelta.end() ? 0.0 : itr->second;
}

double EntityBody::get_num(EntityPropertyNums n) const
{
	return get_base(n) + get_delta(n);
}

void EntityBody::add_delta(EntityPropertyNums n, double amount)
{
	double value = get_delta(n) + amount;
	if (std::abs(value) < 1e-9)
		propsDelta.erase(n);
	else
		propsDelta[n] = value;

	updateInfo = true;
}

void EntityBody::apply_aura(
	size_t sourceId,
	const std::map<EntityPropertyNums, double> &deltas,
	t_seconds expire)
{
	AuraLease &lease = auraLeases[sourceId];

	if (lease.deltas != deltas)
	{
		for (auto &[n, amount] : lease.deltas)
			add_delta(n, -amount);
		for (auto &[n, amount] : deltas)
			add_delta(n, amount);
		lease.deltas = deltas;
	}

	lease.expire = expire;
}

void EntityBody::expire_auras(t_seconds now)
{
	for (auto itr = auraLeases.begin(); itr != auraLeases.end();)
	{
		if (itr->second.expire > now)
		{
			++itr;
			continue;
		}

		for (auto &[n, amount] : itr->second.deltas)
			add_delta(n, -amount);
		itr = auraLeases.erase(itr);
	}
}

void EntityBody::register_kill()
{
	++killCount;

	int threshold = (int)get_num(EntityPropertyNums::BERSERK_KILL_THRESHOLD);
	if (!isBerserk && threshold > 0 && killCount >= threshold)
		isBerserk = true;

	updateInfo = true;
}

bool EntityBody::set_path(PathData &&pathData)
{
	if (!pathData.valid())
	{
		return false;
	}
	this->pathData = pathData;
	this->action = Action::MOVE;
	this->pathData.follower = this->pathData.path.cbegin();

	auto &path = this->pathData.path;

	for (auto itr = path.begin(); std::next(itr, 1) != path.end(); ++itr)
	{
		sf::Vector2f a = vec_tile_to_pos(itr->value);
		sf::Vector2f b = vec_tile_to_pos(std::next(itr, 1)->value);

		bool posBetween, velHeading;
		posBetween = vec_dist(pos, a) + vec_dist(pos, b) < vec_dist(a, b) + 0.5f;
		velHeading = (vel == sf::Vector2f{0.0f, 0.0f})
						 ? true
						 : (math_abs(vec_angle_between(b - a, vel)) < 0.25f);
		if (posBetween && velHeading)
		{
			this->pathData.follower = std::next(itr, 1);
			break;
		}
	}
	return true;
}

void EntityBody::change_action(Action action)
{
	if (action != Action::NONE)
		this->action = action;

	if (spriteWalk != -1)
		change_sprite(spriteWalk);

	switch (action)
	{
	case Action::ATTACKED:
		if (spriteAction != -1)
			change_sprite(spriteHit);
		break;

	case Action::WORK:
		[[fallthrough]];
		/* fallthrough */
	case Action::BUILD:
		[[fallthrough]];
		/* fallthrough */
	case Action::ATTACK:
		if (spriteAction != -1)
			change_sprite(spriteAction);
		break;

	default:
		break;
	}

	reset_timer();
}

void EntityBody::force_stop()
{
	// Stop the entity from moving
	pathData.clear();
	this->set_target(nullptr);
	change_action(EntityBody::Action::IDLE);
}

void EntityBody::reset()
{
	force_stop();
}

void EntityBody::reset_timer()
{
	float time = context->get_time();
	timerAction->reset(time);
	timerPath->reset(time);
}

void EntityBody::logic_aggresive()
{

	switch (action)
	{
	case Action::IDLE:
	{
		while (timerPath->next_surplus(context->get_time()))
		{
			logic_reset();
		}
		break;
	}
	case Action::MOVE:
	{
		if (!this->target)
		{
			logic_reset();
			break;
		}
		EPIC_TEST2()

		// If target moved, recalculate path
		while (timerPath->next_surplus(context->get_time()))
		{
			IVec targetPos = vec_pos_to_tile(target->pos);
			if (targetPos != targetLPos)
			{
				update_path();
			}
		}
		EPIC_TEST2()
		//
		float dist = GameData::bodies_distance(this->target, this);
		if (props.bool_is(EntityPropertyBools::MELEE))
		{
			if (dist < MIN_FOLLOW_DISTANCE)
			{
				pathData.clear();
				change_action(Action::FOLLOW);
			}
		}
		EPIC_TEST2()
		if (props.bool_is(EntityPropertyBools::RANGED))
		{
			if (dist <= MIN_FOLLOW_RANGED_DISTANCE)
			{
				change_action(Action::SHOOT);
				timerAction->reset(context->get_time());
			}
		}
		EPIC_TEST2()
	}
	break;
	case Action::FOLLOW:
	{
		EPIC_TEST2()
		if (!this->target)
		{
			logic_reset();
			break;
		}
		EPIC_TEST2()
		// If the entity confront an enemy, target him
		auto nearest = context->nearest_bodies_radius(
			this->pos,
			this->radius,
			1,
			[this](const GameBody *gb, const int, const int)
			{
				if (GameData::alignment_compare(
						gb->alignment,
						this->alignment) == ALIGNMENTS_ENEMIES)
				{
					return true;
				}
				return false;
			});
		if (nearest.size())
		{
			assert(nearest.front() != dynamic_cast<GameBody *>(this));
			// If near an enemy entity, change target
			set_target(nearest.front());
		}
		EPIC_TEST2()
		float dist = GameData::bodies_distance(target, this);
		EPIC_TEST2()
		if (dist <= MIN_FOLLOW_MELEE_DISTANCE)
		{
			if (this->target->type == BodyType::ENTITY)
			{
				EntityBody *e = dynamic_cast<EntityBody *>(target.get());
				if (e && !e->aggressive)
				{
					e->reset();
					e->change_action(Action::ATTACKED);
				}
			}
			change_action(Action::ATTACK);
			timerAction->reset(context->get_time());
		}
	}
	break;
	case Action::ATTACK:
	{
		while (timerAction->next_surplus(context->get_time()))
		{

			if (!this->target)
			{
				logic_reset();
				break;
			}

			float attack = (float)get_num(EntityPropertyNums::ATTACK);
			if (isBerserk)
				attack *= BERSERK_ATTACK_MULTIPLIER;
			GameData::damage_body(target, attack, this);
			if (target->get_hp() <= 0)
			{
				this->set_target(nullptr);
				logic_reset();
			}

			/*
			else if (this->target->type == BodyType::ENTITY)
			{
				EntityBody* e = dynamic_cast<EntityBody*>(this->target.get());
				assert(e);
				e->hp -= this->attack;
				if (e->hp <= 0)
				{
					this->target = nullptr;
					logic_reset();
					break;
				}
			}
			else if (this->target->type == BodyType::BUILDING)
			{
				BuildingBody* body;
				body = dynamic_cast<BuildingBody*>(target.get());
				assert(body);
				BuildingBase* b = body->base;
				assert(b);
				b->hp -= this->attack;
				if (b->hp <= 0)
				{
					context->queue_remove_build(b);
					this->target = nullptr;
					action = Action::IDLE;
					break;
				}
			}
			*/
		}
		EPIC_TEST2()
		break;
	}
	case Action::SHOOT:
	{
		EPIC_TEST2()
		if (!this->target)
		{
			logic_reset();
			break;
		}
		EPIC_TEST2()
		float dist = GameData::bodies_distance(this->target, this);
		// If the target moved too far, find another target
		if (dist > MIN_FOLLOW_RANGED_DISTANCE)
		{
			logic_reset();
			break;
		}

		EPIC_TEST2()
		while (timerAction->next_surplus(context->get_time()))
		{
			if (spawn.id.group != ENUM_NONE &&
				spawn.serializableID != 0)
			{

				BodyQueueData data;

				data.spawn = this->spawn;
				data.pos = this->pos;
				data.alignment = this->alignment;

				FVec targetPos = target->pos;
				if (target->type == BodyType::BUILDING)
				{
					BuildingBody *build;
					build = dynamic_cast<BuildingBody *>(target.get());
					targetPos = build->pos;
					// build->base->get_center_pos();
				}
				FVec dif = this->pos - targetPos;
				vec_normalize(dif);
				data.vel = -dif;

				// DEBUG("TARGET SET %p", target.get());
				data.target = target;

				context->queue_add_body(data);
			}
		}
		EPIC_TEST2()
		break;
	}
	break;
	default:
		break;
	}
	EPIC_TEST2()
}

void EntityBody::update_path()
{
	const int dijkstra = (int)context->get_const(
		t_constnum::A_STAR_DIJKSTRA_VALUE);
	const int greed = (int)context->get_const(
		t_constnum::A_STAR_GREED_VALUE);
	set_path(generate_path(
		vec_pos_to_tile(this->pos),
		vec_pos_to_tile(target->pos),
		context->chunks,
		dijkstra,
		greed,
		-1.f));
}

int EntityBody::get_direction_frame(int rotation)
{
	const float CALIBRATION_OFFSET = -1/16.f;

	sf::Vector2f dir = vel;
	// Code made by Claude Sonnet 5 - while attacking or flinching from a hit,
	// vel no longer reflects "walking toward the target" (it's zeroed by the
	// action switch, then nudged around by separation jitter from nearby
	// bodies), so always face the target directly instead of trusting vel
	if (target &&
		(action == Action::ATTACK || action == Action::ATTACKED))
	{
		dir = target->pos - this->pos;
	}

	sf::Vector2f screenVel = { dir.x - dir.y, dir.x + dir.y };
	float angle = atan2f(-screenVel.y, screenVel.x);
	angle -= (float)rotation * (float)M_PI_2;
	angle -= CALIBRATION_OFFSET;
	angle = math_mod(angle, (float)M_2PI);
	int row = (int)(angle / ((float)M_2PI / 8.f) + 0.5f);
	row += 3;
	return row % 8;
}

void EntityBody::update(float delta)
{
	const float MAX_DELTA = 1.f;
	bool ignoreBarriers = props.bool_is(
							  EntityPropertyBools::CAN_PHASE) &&
						  0;

	float speedScale = 1.f;
	delta = std::min(delta, MAX_DELTA);

	// If hp depleted, entity is dead
	if (get_hp() <= 0 || dead)
	{
		// Code made by Claude Sonnet 5 - this branch re-executes every
		// frame while a dead entity lingers before cleanup, so the death
		// must only be recorded once, on the alive-to-dead edge
		if (!dead)
			context->record_death(this);
		dead = true;
		return;
	}

	expire_auras(context->get_time());

	// Get  tile speed bonus, if available
	{
		sf::Vector2i tilePos = vec_pos_to_tile(this->pos);
		Tile &tile = context->chunks->get_tile(tilePos.x, tilePos.y);
		if (!tile.is_barrier() && tile.building)
		{
			BuildingBase *b = tile.building->base;
			if (b->props.num_is(PropertyNum::SPEED_BONUS))
				speedScale = ((float)b->props.num_get(PropertyNum::SPEED_BONUS));
		}
	}

	//				Calculate velocity

	float maxSpeed =
		(float)get_num(EntityPropertyNums::SPEED_MULTIPLIER) *
		this->maxSpeed;

	if (debugFollowMouse)
	{
		FVec dif = debugTarget - this->pos;
		if (vec_lensq(dif) > 0.0001f)
		{
			vec_normalize(dif);
			vel = dif * maxSpeed;
		}
		else
		{
			vel = {0.f, 0.f};
		}
	}
	else
	{
	FVec destination;
	FVec force;
	// Calculate path following direction
	if (action == Action::MOVE &&
		pathData.valid() &&
		!pathData.finished())
	{
		destination = vec_tile_to_pos(pathData.front());
		if (vec_distsq(destination, this->pos) < 0.01f)
			pathData.pop();
	}

	//
	if (action == Action::FOLLOW && this->target)
	{
		destination = this->target->pos;
	}

	switch (action)
	{
	case Action::IDLE:
	{
		// todo convert to timer
		t_seconds t = localClock->time_passed(
			context->get_time());

		float x = noise.GetNoise(
			t / 4.f + objectId / 2.472f,
			(float)id,
			1.f);
		float y = noise.GetNoise(
			t / 4.f + objectId / 2.472f,
			(float)id,
			2.f);

		force = {x, y};
		vec_normalize(force);
		force = force * maxForce / 16.f;
	}

	break;
	case Action::MOVE:
	case Action::FOLLOW:
	{
		FVec deltaPath = destination - this->pos;
		vec_normalize(deltaPath);

		if (destination != FVec{0.0f, 0.0f})
		{
			// Steering
			deltaPath *= maxSpeed;
			force = deltaPath - vel;
			if (maxForce == 0.0f || maxSpeed == 0.0f)
				force = FVec{0.0f, 0.0f};
			else
				force *= maxForce / maxSpeed;
		}
		// Linear movement
		if (false)
			this->vel = deltaPath * this->maxSpeed;
	}
	break;
	default:
		vel = {0.f, 0.f};
		break;
	}

	// Seperation
	if (true)
	{
		std::list<GameBody *> nearest =
			context->nearest_bodies_radius(
				this->pos,
				radius);

		FVec dif;
		FVec seperationForce = {0.0f, 0.0f};
		int counter = 0;
		nearbyBuilds.clear();
		for (auto &body : nearest)
		{
			FVec pos = {0.0f, 0.0f};
			float dist = 0.0f;
			if (body->type == BodyType::ENTITY)
			{
				pos = body->pos;
			}
			else if (body->type == BodyType::BUILDING && !ignoreBarriers)
			{
				BuildingBody *b = dynamic_cast<BuildingBody *>(body);
				nearbyBuilds.push_back(b);

				if (!b->base->props.bool_is(PropertyBool::BARRIER))
					continue;

				pos = aabb_closest_point(
					body->pos,
					FVec{1.f, 1.f},
					this->pos);
			}

			dist = vec_distsq(pos, this->pos);
			if (dist > 0.0f && dist <= radius * radius)
			{
				FVec dif = this->pos - pos;
				float len = vec_normalize(dif);
				FVec thisForce = dif * (1.f / len);

				if (vec_lensq(this->vel) < (this->maxSpeed * this->maxSpeed) / 2.f)
				{
					thisForce *= 50.2f;
				}
				else
				{
					thisForce *= 0.8f;
				}

				seperationForce += thisForce;

				++counter;
			}
		}

		// Average seperation force
		if (counter)
			force += seperationForce / (float)counter;
	}

	// Apply force
	vel += force * delta;

	// Limit velocity
	if (vec_lensq(vel) > maxSpeed * maxSpeed)
	{
		vec_normalize(vel);
		vel *= maxSpeed;
	}
	}

	bool moved = vel != sf::Vector2f{0.f, 0.f};

	if (moved)
	{
		sf::Vector2f deltaVel = speedScale * this->vel;

		this->animTime += vec_len(deltaVel) * 4;
		this->animFrame = (int)(this->animTime);

		sf::Vector2f oldPos = this->pos;
		sf::Vector2f newPos = oldPos + deltaVel;

		// Pre movement collision
		bool move = !context->body_is_collision(newPos, rectSize);

		if (move || ignoreBarriers)
		{
			context->move_entity(this, oldPos, newPos);
		}
	}
	// Code made by Claude Sonnet 5 - work/attack/hit animations have no velocity to drive them, so give them their own steady clock instead of freezing on the last walk frame
	else if (action == Action::WORK || action == Action::BUILD || action == Action::ATTACK || action == Action::ATTACKED)
	{
		this->animTime += STATIONARY_ANIM_SPEED * delta;
		this->animFrame = (int)(this->animTime);
	}

	// Code made by Claude Sonnet 5 - integrate and decay the knockback from a hit, stopping at barriers
	if (vec_lensq(knockVel) > 0.0001f)
	{
		sf::Vector2f oldPos = this->pos;
		sf::Vector2f newPos = oldPos + knockVel * delta;
		if (!context->body_is_collision(newPos, rectSize))
			context->move_entity(this, oldPos, newPos);
		else
			knockVel = {0.f, 0.f};

		knockVel *= std::max(0.f, 1.f - 8.f * delta);
	}
	else
		knockVel = {0.f, 0.f};

	logic();

	// Post movement resolve collision
	if (!ignoreBarriers)
	{
		sf::Vector2f ogPos = pos;
		int count = 0;
		sf::Vector2i collPos;
		while ((context->body_is_collision(pos, rectSize, &collPos)) && count < 16)
		{
			sf::Vector2f dif = {
				(float)prng_get_double(),
				(float)prng_get_double()}; // (sf::Vector2f)collPos + sf::Vector2f{.5f, .5f} - this->pos;
			vec_normalize(dif);
			this->pos += dif;
			++count;
		}
		if (count)
			context->move_entity(this, ogPos, pos);
	}

	IVec tilePos = vec_pos_to_tile(this->pos);
	// Update path on grid change
	EntityBody *e = dynamic_cast<EntityBody *>(this);
	if ((context->changesContext.addedTiles.size() ||
		 context->changesContext.removedTiles.size()) &&
		!context->is_barrier(tilePos) &&
		e->pathData.valid())
	{
		Tile *d = e->pathData.destination;
		const int dijkstra = (int)context->get_const(
			t_constnum::A_STAR_DIJKSTRA_VALUE);
		const int greed = (int)context->get_const(
			t_constnum::A_STAR_GREED_VALUE);
		auto path = generate_path(
			vec_pos_to_tile(e->pos),
			d->tilePos,
			context->chunks,
			dijkstra,
			greed,
			-1.f);
		e->pathData.clear();

		// Is the path leading to the destination
		if (path.valid())
		{
			e->set_path(std::move(path));
		}
		else
		{
			if (d && d->building && d->building->base.is_valid())
			{
				auto &list = d->building->base->storedEntities;
				auto itr = std::find(list.begin(), list.end(), e);
				if (itr != list.end())
				{
					list.erase(itr);
				}
			}
			pathData.clear();
		}
	}
}

json EntityBody::to_json() const
{
	json ret = GameBody::to_json();

#define j ret

	j["entityType"] = entityType;
	j["id"] = id;
	j["hp"] = hp;
	j["killCount"] = killCount;
	j["isBerserk"] = isBerserk;
	j["animTime"] = animTime;
	// j["nearbyBuilds"] = nearbyBuilds;
	j["pathData"] = pathData;
	j["inventorySize"] = inventorySize;
	j["transferSize"] = transferSize;
	j["maxSpeed"] = maxSpeed;
	j["maxForce"] = maxForce;
	j["updateInfo"] = updateInfo;
	j["targetLPos"] = targetLPos;
	j["props"] = props;
	j["timerAction"] = timerAction;
	j["timerPath"] = timerPath;
	j["localClock"] = localClock;
	j["action"] = action;
	j["spawn"] = spawn;

#undef j

	to_json_derived(ret);

	return ret;
}

void EntityBody::from_json(const json &j)
{
	try
	{
		GameBody::from_json(j);
		j.at("entityType").get_to(entityType);
		j.at("id").get_to(id);
		j.at("hp").get_to(hp);
		if (j.count("killCount"))
			j.at("killCount").get_to(killCount);
		if (j.count("isBerserk"))
			j.at("isBerserk").get_to(isBerserk);
		j.at("animTime").get_to(animTime);
		// j.at("nearbyBuilds").get_to(nearbyBuilds);
		j.at("pathData").get_to(pathData);
		j.at("inventorySize").get_to(inventorySize);
		j.at("transferSize").get_to(transferSize);
		j.at("maxSpeed").get_to(maxSpeed);
		j.at("maxForce").get_to(maxForce);
		j.at("updateInfo").get_to(updateInfo);
		j.at("targetLPos").get_to(targetLPos);
		j.at("props").get_to(props);

		if (!timerAction.get())
			timerAction = t_time_manager::make_independent_timer();
		if (j.count("timerAction"))
			j.at("timerAction").get_to(timerAction);

		if (!timerPath.get())
			timerPath = t_time_manager::make_independent_timer();
		if (j.count("timerPath"))
			j.at("timerPath").get_to(timerPath);

		if (j.count("localClock"))
			j.at("localClock").get_to(localClock);

		j.at("action").get_to(action);
		j.at("spawn").get_to(spawn);
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize BuildingBody: %s", e.what());
	}

	from_json_derived(j);
}

void EntityBody::serialize_publish(const SerializeMap &map)
{
	GameBody::serialize_publish(map);

	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);

	g->add_timer(timerAction);
	g->add_timer(timerPath);
	g->add_timer(localClock);

	localClock->init(g->get_time());
	timerAction->init(context->get_time());
	timerPath->init(context->get_time());

	for (auto &v : nearbyBuilds)
		map.apply(v);

	// Path data doesn't desiarilized in game_data.cpp
	if (pathData.path.size())
		pathData.serialize_publish(map);

	serialize_publish_derived(map);
}

void EntityBody::serialize_initialize(const SerializeMap &map)
{
	GameBody::serialize_initialize(map);

	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);

	// Path data doesn't desiarilized in game_data.cpp
	pathData.serialize_initialize(map);

	serialize_initialize_derived(map);
}

EntityEnemy::EntityEnemy()
	: EntityEnemy(nullptr, {0.f, 0.f}, 0, 0)
{
}

EntityEnemy::EntityEnemy(
	GameData *context,
	const sf::Vector2f &pos,
	t_sprite sprite,
	size_t id)
	: EntityBody(
		  context,
		  pos,
		  sprite,
		  SERIALIZABLE_ENEMY,
		  id)
{
	this->entityType = EntityType::ENEMY;

	timerAction->set_length(2.f);
	// Recalculate path every 1 second (if needed)
	timerPath->set_length(1.f);

	this->alignment = ALIGNMENT_ENEMY;
}

bool EntityEnemy::confirm_body_derived(GameData *context)
{
	return true;
}

void EntityEnemy::logic_reset()
{
	this->search_entity();
}

void EntityEnemy::logic()
{

	this->logic_aggresive();
}

void EntityEnemy::update(float delta)
{
	EntityBody::update(delta);
}

void EntityEnemy::to_json_derived(json &j) const
{
	j["type"] = enemyType;
}

void EntityEnemy::from_json_derived(const json &j)
{
	try
	{
		j.at("type").get_to(enemyType);
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize EntityEnemy: %s", e.what());
	}
}

void EntityEnemy::serialize_publish_derived(const SerializeMap &map)
{
}

void EntityEnemy::serialize_initialize_derived(const SerializeMap &map)
{
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);
	// this->objectId = g->counterContext.objectIdCounter.advance<EntityEnemy>();
	this->confirm_body(g);
}

//

EntityCitizen::EntityCitizen()
	: EntityCitizen(nullptr, {0.f, 0.f}, 0, 0)
{
}

EntityCitizen::EntityCitizen(
	GameData *context,
	const sf::Vector2f &pos,
	t_sprite sprite,
	size_t id)
	: EntityBody(
		  context,
		  pos,
		  sprite,
		  SERIALIZABLE_CITIZEN,
		  id)
{
	this->entityType = EntityType::CITIZEN;
	// maxSpeed += (float)prng_get_double() * 0.1f;

	timerAction->set_length(2.f);
	// Recalculate path every 1 second (if needed)
	timerPath->set_length(1.f);

	this->alignment = ALIGNMENT_FRIENDLY;
}

bool EntityCitizen::confirm_body_derived(GameData *context)
{
	assert(context);
	context->bodiesContext.entityCitizens.push_back(this);
	assert(context->bodiesContext.entityCitizens.size());

	if (context)
	{
		// this line got replaced in the one in apply_data_derived
		// this->job = CitizenJob::NONE;
		this->rInventoryCap =
			Resources::res_inf(&context->resourceContext.weights);
	}
	else
	{
		DEBUG("Inventory cap uninitialized!");
	}

	return true;
}

void EntityCitizen::apply_data_derived(GameBodyConfig *config)
{
	if (static_cast<EntityStats *>(config) == nullptr)
	{
		WARNING(
			"In function %s invalid type of input function parameter, should be of type GameBodyConfig",
			__PRETTY_FUNCTION__);
	}

	auto &stats = *static_cast<EntityStats *>(config);

	this->job = (CitizenJob)stats.type.id;
}

void EntityCitizen::to_json_derived(json &j) const
{
	j["job"] = job;
	if (home)
		j["home"] = home;
	if (workplace)
		j["workplace"] = workplace;
	j["insideWorkplace"] = insideWorkplace;
	j["rInventory"] = rInventory;
	j["nextAction"] = nextAction;
	j["rActionBool"] = rActionBool;
}

void EntityCitizen::from_json_derived(const json &j)
{
	try
	{
		j.at("job").get_to(job);
		if (j.count("home"))
			j.at("home").get_to(home);
		if (j.count("workplace"))
			j.at("workplace").get_to(workplace);
		j.at("insideWorkplace").get_to(insideWorkplace);
		j.at("rInventory").get_to(rInventory);
		j.at("nextAction").get_to(nextAction);
		j.at("rActionBool").get_to(rActionBool);
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize BuildingBody: %s", e.what());
	}
}

void EntityCitizen::serialize_publish_derived(const SerializeMap &map)
{
	if (home.is_valid())
		map.apply(home);
	if (workplace.is_valid())
	{
		map.apply(workplace);
	}
}

void EntityCitizen::serialize_initialize_derived(const SerializeMap &map)
{
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);
	// this->objectId = g->counterContext.objectIdCounter.advance<EntityCitizen>();
	this->confirm_body(g);
}

void EntityCitizen::update(float delta)
{
	if (insideWorkplace)
	{
		logic();
		return;
	}

	EntityBody::update(delta);
}

bool EntityCitizen::inventory_full(float precentageLimit) const
{
	// Zero capacity meaning always full
	if (inventorySize == 0)
		return true;
	// Infinite / undefined storage size
	if (inventorySize < 0)
		return false;
	return (float)rInventory.weight(&context->resourceContext.weights) /
			   inventorySize >
		   precentageLimit;
}

void EntityCitizen::logic()
{
	Resources prevInventory = this->rInventory;
	int prevHp = this->hp;
	Action prevAction = this->action;

	if (job == CitizenJob::TEST01)
	{
		logic_test01();
	}
	else if (aggressive)
	{
		logic_aggresive();
	}
	else
	{
		logic_worker();
	}

	if (this->rInventory != prevInventory ||
		this->hp != prevHp ||
		this->action != prevAction)
	{
		this->updateInfo = true;
	}
}

// Workplace outputs stuff and has free space
#define HAS_OUT_SPACE()                                 \
	(workplace->rOut.empty() ||                         \
	 (workplace->entities.size() &&                     \
	  Resources::can_transfer(                          \
		  workplace->rOut * workplace->entities.size(), \
		  workplace->rStorage,                          \
		  &(context->resourceContext.weights),  \
		  workplace->weightCap,                         \
		  workplace->rStoreCap))) // Max storage capacity

void EntityCitizen::logic_reset()
{
	// This class is for friendly entities only!
	assert(this->alignment == ALIGNMENT_FRIENDLY);

	this->nextAction = Action::IDLE;

	if (this->aggressive && search_entity())
	{
		return;
	}

	// Workers have a different logic reset pattern
	if (workplace != nullptr)
	{
		logic_reset_worker();
		return;
	}

	PathData pathData;
	auto condition =
		[](const sf::Vector2i &, const GameBody *gb, const int)
	{
		// Sometimes a building's entityLimit can be changed mannualy in
		// the init function, so the "NEEDS_WORKERS" tag might be
		// innacurate.
		const BuildingBody *bb = dynamic_cast<const BuildingBody *>(gb);
		assert(bb);
		BuildingBase *b = bb->base;
		if (b->entities.size() >= b->entityLimit)
		{
			return false;
		}
		return true;
	};

	if (job != CitizenJob::NONE)
		return;

	PathData newPath = find_building_and_path(
		context,
		pos,
		{ENUM_INGAME_PROPERTIES, (t_id)IngameProperties::NEEDS_WORKERS},
		-1.f,
		std::move(condition));

	if (newPath.valid())
	{
		// ONLY accept the job if we actually have a way to get there
		newPath.destination->building->base->accept_entity(this);
		nextAction = Action::GET_JOB;
		set_path(std::move(newPath));
	}
	else
	{
		DEBUG("InvalidPath %s! Entity %d. Line: %d",
			  newPath.why_invalid().c_str(), (int)objectId, __LINE__);
		// Ensure we don't have a partial job state assigned
		this->job = CitizenJob::NONE;
		this->workplace = nullptr;
	}
}

PathData EntityCitizen::generate_path_worker(const IVec &end)
{
	const int dijkstra = (int)context->get_const(
		t_constnum::A_STAR_DIJKSTRA_VALUE);
	const int greed = (int)context->get_const(
		t_constnum::A_STAR_GREED_VALUE);
	return generate_path(
		vec_pos_to_tile(this->pos), // workplace->tilePos,
		end,
		context->chunks,
		dijkstra,
		greed,
		(workplace && 0 ? workplace->effectRadius : -1.f));
}


void EntityCitizen::recalculate_worker_stats()
{
	// This function was made by AI

	bool success = false;
	EntityStats *baseStats = context->get_entity_stats(ENUM_CITIZEN_JOB, (t_id)this->job, &success);
	if (!success || !baseStats) return;

	float calcHp = baseStats->hp;
	float calcAttack = (float)baseStats->props.num_get_or(EntityPropertyNums::ATTACK, 1.0);
	float calcSpeed = baseStats->maxSpeed;
	float calcActionCooldown = baseStats->cooldownAction;

	// 2. Fetch the source building (prioritize workplace, fallback to home)
	BuildingBase* sourceBuild = this->workplace ? this->workplace.get() : (this->home ? this->home.get() : nullptr);

	if (sourceBuild)
	{
		auto& p = sourceBuild->props;
		typedef PropertyNum PN;

		// Apply Overrides First
		if (p.num_is(PN::ENTITY_HP_OVERRIDE)) calcHp = (float)p.num_get(PN::ENTITY_HP_OVERRIDE);
		if (p.num_is(PN::ENTITY_ATTACK_OVERRIDE)) calcAttack = (float)p.num_get(PN::ENTITY_ATTACK_OVERRIDE);

		// Apply Multipliers Second
		if (p.num_is(PN::ENTITY_HP_RATIO)) calcHp *= (float)p.num_get(PN::ENTITY_HP_RATIO);
		if (p.num_is(PN::ENTITY_ATTACK_RATIO)) calcAttack *= (float)p.num_get(PN::ENTITY_ATTACK_RATIO);
		if (p.num_is(PN::ENTITY_SPEED_RATIO)) calcSpeed *= (float)p.num_get(PN::ENTITY_SPEED_RATIO);
		if (p.num_is(PN::ENTITY_ACTION_TIME_RATIO)) calcActionCooldown *= (float)p.num_get(PN::ENTITY_ACTION_TIME_RATIO);
	}

	// 3. Apply finalized stats
	this->hp = (int)calcHp;
	this->props.num_set(EntityPropertyNums::ATTACK, (double)(int)calcAttack);
	this->maxSpeed = calcSpeed;
	if (calcActionCooldown > 0.0f) {
		this->timerAction->set_length(calcActionCooldown);
	}
}

void EntityCitizen::logic_reset_worker()
{
	auto &resourceWeights = context->resourceContext.weights;

	bool bIn = !workplace->rIn.empty();
	bool bOut = !workplace->rOut.empty();
	t_globalenum destType = 0;

	timerAction->reset(context->get_time());
	timerPath->reset(context->get_time());

	reset();

	const int dijkstra = (int)context->get_const(
		t_constnum::A_STAR_DIJKSTRA_VALUE);
	const int greed = (int)context->get_const(
		t_constnum::A_STAR_GREED_VALUE);

	// If inventory has useless resources dump them somewhere
	auto inReverse =
		workplace->rIn.reverse_bool(&resourceWeights);
	// Also, this behvoiur does not apply to couriers
	if (rInventory.has_bool(inReverse) && this->job != CitizenJob::COURIER)
	{
		t_id storageId = (t_id)PropertyBool::ANY_STORAGE;
		// If the character's a courier, move stuff from buildings to destinated
		// main storage.
		PathData path = find_building_and_path(
			context,
			this->pos,
			t_idpair{ENUM_PROPERTY_BOOL, storageId},
			workplace->effectRadius,
			[this, &resourceWeights](const sf::Vector2i &, const GameBody *gb, int)
			{
				const BuildingBase *b =
					dynamic_cast<const BuildingBody *>(gb)->base;
				return //! b->rStorage.empty() &&
					Resources::can_transfer(rInventory, b->rStorage,
											&resourceWeights,
											b->weightCap, b->rStoreCap,
											workplace->rIn.reverse_bool(
												resourceWeights));
			});
		if (path.valid())
		{
			nextAction = Action::TRANSFER;
			rActionBool = inReverse;
			go_to_building(path);
			return;
		}
	}

	// Does the entity's workplace has free space to be filled with the
	// building's output production
	bool hasOutspace = HAS_OUT_SPACE();

	// If the workplace is full of output go gather
	// from from it.
	if (bOut)
	{
		/*DEBUG("%d, %d\nWrkStr: %s, INV: %s, Limit: %d, InvCap: %s",
			  hasOutspace,
			  Resources::can_transfer(
				  workplace->rStorage,
				  this->rInventory,
				  &context->resourceContext.resourceWeights,
				  inventorySize,
				  this->rInventoryCap),
			  CSTR(workplace->rStorage),
			  CSTR(this->rInventory),
			  (int)(inventorySize),
			  CSTR(this->rInventoryCap));*/

		if (!hasOutspace &&
			Resources::can_transfer(
				workplace->rStorage,
				this->rInventory,
				&context->resourceContext.weights,
				inventorySize,
				this->rInventoryCap))
		{
			auto path = generate_path_worker(
				workplace->tilePos);
			if (path.valid())
			{
				nextAction = Action::COLLECT;
				rActionBool = workplace->rIn.reverse_bool(
					&context->resourceContext.weights);
				go_to_building(path);
				return;
			}
		}
	}

	// If the workplace requires resources and the entity
	// haves them or the workplace haves them,
	// dump them to the workplace.
	if (bIn)
	{
		Resources input = workplace->rIn * workplace->entities.size();
		/*DEBUG("In: %s, Storage: %s, Inv: %s. %d %d",
			  CSTR(input),
			  CSTR(workplace->rStorage),
			  CSTR(rInventory),
			  workplace->rStorage.affordable(input),
			  rInventory.affordable(input));*/
		bool bICanWord = workplace->rStorage.affordable(input);
		bool bICanGive = rInventory.affordable(input);
		if (bICanWord || bICanGive)
		{
			auto path = generate_path_worker(
				workplace->tilePos);
			if (path.valid())
			{
				// I have resources so just go dump them
				if (bICanGive)
				{
					nextAction = Action::TRANSFER;
					rActionBool = workplace->rIn.to_bool();
				}
				// Workplace have resources so just go inside
				else if (bICanWord)
					nextAction = Action::WORK;
				go_to_building(path);
				return;
			}
		}
	}

	if (bIn)
	{
		// If workplace and entity dont have enough go gather
		if (!inventory_full())
		{
			auto path = find_building_and_path(
				context,
				this->pos,
				{ENUM_PROPERTY_BOOL, (t_id)PropertyBool::ANY_STORAGE},
				workplace->effectRadius,
				[this](const sf::Vector2i, const GameBody *gb, const int)
				{
					const BuildingBase *b =
						dynamic_cast<const BuildingBody *>(gb)->base;
					bool x = Resources::can_transfer(
						b->rStorage,
						rInventory,
						&context->resourceContext.weights,
						inventorySize,
						rInventoryCap,
						workplace->rIn);
					return x;
				});
			if (path.valid())
			{
				nextAction = Action::COLLECT;
				rActionBool = workplace->rIn.to_bool();

				go_to_building(path);
				return;
			}
		}
	}

	// Building generates resources and has space for those
	if (bOut && hasOutspace)
	{
		auto path = generate_path_worker(
			workplace->tilePos);
		if (path.valid())
		{
			nextAction = Action::WORK;

			go_to_building(path);
			return;
		}
	}

	// If destType not zero
	if (destType)
		return;

	// Go search for specific type of building
	// and do a specific action
	if (workplace->job == (size_t)CitizenJob::GATHERER)
	{
		if (!inventory_full())
		{
			PathData path = find_build_path(
				context,
				context->chunks,
				this->pos,
				workplace->get_center_pos(),
				t_idpair{ENUM_PROPERTY_BOOL, (t_id)PropertyBool::HARVESTABLE},
				workplace->effectRadius,
				[this, &resourceWeights](sf::Vector2i, const GameBody *gb, int)
				{
					const BuildingBase *b = dynamic_cast<const BuildingBody *>(gb)->base;
					assert(b);
					return !b->rStorage.empty() &&
						   Resources::can_transfer_weight(
							   b->rStorage,
							   rInventory,
							   &resourceWeights,
							   inventorySize);
				});
			if (path.valid())
			{
				nextAction = Action::COLLECT;
				rActionBool = path.destination->get_building()->rStorage.to_bool();
				
				go_to_building(path);
				return;
			}
		}
	}
	else if (workplace->job == (size_t)CitizenJob::BUILDER)
	{
		PathData path = find_building_and_path(
			context,
			this->pos,
			t_idpair{ENUM_BUILDING_TYPE, (t_id)BuildingType::CONSTRUCTION},
			workplace->effectRadius,
			[this](sf::Vector2i, const GameBody *gb, int)
			{
				BuildingBase *b = dynamic_cast<const BuildingBody *>(gb)->base;
				assert(b);
				auto &data = context->constructions.at(b);
				return data.work_left() > 0;
			});
		if (path.valid())
		{
			nextAction = Action::BUILD;
			go_to_building(path);
			return;
		}
	}
	else if (this->job == CitizenJob::COURIER)
	{

		PathData path;

		// If the entity carrying any resources
		if (!rInventory.empty())
		{
			// Find a building that needs the resources the entity has
			path = find_building_and_path(
				context,
				this->pos,
				t_idpair{ENUM_PROPERTY_BOOL, (t_id)PropertyBool::WORKPLACE},
				workplace->effectRadius,
				[this](const sf::Vector2i &, const GameBody *gb, int)
				{
					const BuildingBase *b =
						dynamic_cast<const BuildingBody *>(gb)->base;
					if (!rInventory.has_bool(b->rIn))
						return false;
					return Resources::can_transfer(rInventory, b->rStorage,
												   &context->resourceContext.weights,
												   b->weightCap, b->rStoreCap,
												   b->rIn);
				});
			if (path.valid())
			{
				nextAction = Action::TRANSFER;
				rActionBool = path.destination->get_building()->rIn;
				go_to_building(path);
				return;
			}

			// Find a storage building with space

			path = find_building_and_path(
				context,
				this->pos,
				t_idpair{ENUM_PROPERTY_BOOL, (t_id)PropertyBool::STORAGE},
				workplace->effectRadius,
				[this](const sf::Vector2i &, const GameBody *gb, int)
				{
					const BuildingBase *b =
						dynamic_cast<const BuildingBody *>(gb)->base;
					return //! b->rStorage.empty() &&
						Resources::can_transfer(rInventory, b->rStorage,
												&context->resourceContext.weights,
												b->weightCap, b->rStoreCap,
												workplace->rIn.reverse_bool(
													context->resourceContext.weights));
				});
			if (path.valid())
			{
				nextAction = Action::TRANSFER;
				rActionBool = Resources::all(1, &context->resourceContext.weights);
				go_to_building(path);
				return;
			}
		}

		if (!inventory_full())
		{

			// Move stuff from temporary storage to storage
			path = find_building_and_path(
				context,
				this->pos,
				t_idpair{ENUM_PROPERTY_BOOL, (t_id)PropertyBool::TMP_STORAGE},
				workplace->effectRadius,
				[](sf::Vector2i, const GameBody *gb, int)
				{
					const BuildingBase *b = dynamic_cast<const BuildingBody *>(gb)->base;
					assert(b);
					if (b->alignment != ALIGNMENT_FRIENDLY)
						return false;
					return !b->rStorage.empty();
				});
			if (path.valid())
			{
				nextAction = Action::COLLECT;
				rActionBool = path.destination->get_building()->rStorage.to_bool();

				go_to_building(path);
				return;
			}

			// Move stuff out of workplaces
			path = find_building_and_path(
				context,
				this->pos,
				t_idpair{ENUM_PROPERTY_BOOL, (t_id)PropertyBool::WORKPLACE},
				workplace->effectRadius,
				[](sf::Vector2i, const GameBody *gb, int)
				{
					const BuildingBase *b = dynamic_cast<const BuildingBody *>(gb)->base;
					assert(b);
					// If the building has resources, and it doesn't use them
					return !b->rStorage.empty() &&
						   b->rStorage.has_bool(
							   b->rIn.reverse_bool(b->context->resourceContext.weights));
				});
			if (path.valid())
			{
				nextAction = Action::COLLECT;
				rActionBool = path.destination->get_building()->rStorage.to_bool();

				go_to_building(path);
				return;
			}

			float cloggedPercs;
			cloggedPercs = context->get_const(
				ConstantFloating::EMPTY_INVENTORY_PERCENTAGE);

			// Move stuff into workplaces
			path = find_building_and_path(
				context,
				this->pos,
				t_idpair{ENUM_PROPERTY_BOOL, (t_id)PropertyBool::WORKPLACE},
				workplace->effectRadius,
				[cloggedPercs](sf::Vector2i, const GameBody *gb, int)
				{
					const BuildingBase *b = dynamic_cast<const BuildingBody *>(gb)->base;
					assert(b);
					// If the building does not requier resources to function,
					// no need to find resources for it
					if (b->rIn.empty())
						return false;
					// If the buildings resources take less than
					// "CLOGGED_INVENTORY_PERCENTAGE" storage space,
					// that it should be filled.
					float capA = b->rStorage.get_presentage_of(
						b->rStoreCap);
					float capB = b->rStorage.get_presentage_of(
						b->weightCap, &b->context->resourceContext.weights);
					float maxCap = math_max(capA, capB);
					// todo replace arbitrery
					return (maxCap < cloggedPercs);
				});

			// If a reachable building that requiers resources found
			if (path.valid())
			{
				// Find a storage with these resources
				PathData path2 = find_building_and_path(
					context,
					this->pos,
					t_idpair{ENUM_PROPERTY_BOOL, (t_id)PropertyBool::ANY_STORAGE},
					workplace->effectRadius,
					[path](sf::Vector2i, const GameBody *gb, int)
					{
						const BuildingBase *b = dynamic_cast<const BuildingBody *>(gb)->base;
						assert(b);
						// If the building has resources, and it doesn't use them
						return !b->rStorage.empty() &&
							   b->rStorage.has_bool(
								   path.destination->get_building()->rIn);
					});

				if (path2.valid())
				{
					DEBUG("%d %s\n", (int)objectId, path2.to_string().c_str());

					nextAction = Action::COLLECT;
					rActionBool = path.destination->get_building()->rIn.to_bool();

					set_path(std::move(path2));
					return;
				}
			}
		}
	}

	// Go to WORK
	if (workplace->props.bool_is(PropertyBool::INSIDE))
	{
		PathData pathData = generate_path_worker(
			workplace->tilePos);
		if (pathData.valid())
		{
			nextAction = Action::WORK;
			set_path(std::move(pathData));
		}
	}
}

void EntityCitizen::logic_worker()
{
	t_weights& weights = context->resourceContext.weights;

	switch (action)
	{
	case Action::IDLE:
	{
		while (timerPath->next_surplus(context->get_time()))
			logic_reset();

		break;
	}
	break;
	case Action::MOVE:
	{
		// If path is invalid, reset
		auto destination = pathData.destination;
		if (!destination || !destination->building || !pathData.valid())
		{
			while (timerPath->next_surplus(context->get_time()))
				logic_reset();
			break;
		}

		// Get nearest buildings
		auto &nearest = this->nearbyBuilds;

		auto itr = nearest.begin();
		assert(destination);

		// Check if the destination in one of the nearby buildings
		auto itrWorkplace = std::find(nearest.begin(), nearest.end(), destination->building);
		if (!destination->building ||
			(itr = itrWorkplace) == nearest.end())
		{
			break;
		}

		// Once a nearby building was detected
		BuildingBody *body = *itr;
		nearest.pop_front();
		BuildingBase *build = body->base;

		// If the entity got here for a job, get it
		if (nextAction == Action::GET_JOB &&
			build->props.bool_is(PropertyBool::WORKPLACE))
		{
			build->accept_entity_job(this);
			nextAction = Action::IDLE;
		}

		// If the place is a workplace, enter.
		if ((nextAction == Action::WORK ||
			 workplace->props.bool_is(PropertyBool::INSIDE)) &&
			build->props.bool_is(PropertyBool::WORKPLACE))
		{
			workplace->enter_entity(this);
			if (!insideWorkplace)
				logic_reset();
		}
		else
			change_action(nextAction);

		// If the entity still idling, reset
		if (action == Action::IDLE)
			logic_reset();

		break;
	}
	break;
	case Action::WORK:
	{
		while (timerPath->finished(context->get_time()))
		{
			timerPath->reset(context->get_time());

			// If the workplace is out of resources
			if (!workplace->rStorage.affordable(
					workplace->rIn * workplace->entities.size()))
			{
				workplace->updateInfo = true;
				// If the citizen can tranfer resources to the
				// workplace's inventory, do that.
				if (this->rInventory.has_bool(workplace->rIn) &&
					Resources::can_transfer(
						this->rInventory,
						workplace->rStorage,
						&weights,
						workplace->weightCap,
						workplace->rStoreCap))
				{
					Resources workplaceStorageBefore = workplace->rStorage;
					Resources::transfer(
						this->rInventory,
						workplace->rStorage,
						&weights,
						transferSize,
						workplace->rStoreCap,
						workplace->weightCap);
					context->record_resource_gain_delta(workplaceStorageBefore, workplace->rStorage);
				}
				else
				{
					// If all resources are gone, go outside.
					workplace->exit_entity(this);
					logic_reset();
					break;
				}
			}

			// If the workplace out of space
			if (!HAS_OUT_SPACE())
			{
				if (Resources::can_transfer(
						workplace->rStorage,
						this->rInventory,
						&context->resourceContext.weights,
						inventorySize,
						rInventoryCap))
				{
					Resources::transfer(
						workplace->rStorage,
						this->rInventory,
						&context->resourceContext.weights,
						transferSize,
						rInventoryCap,
						inventorySize);
					break;
				}
				else
				{
					// If all resources are gone, go outside.
					workplace->exit_entity(this);
					logic_reset();
					break;
				}
			}
		}
	}
	break;
	case Action::COLLECT:
	{
		auto destination = pathData.destination;
		if (!destination || !destination->building || !pathData.valid())
		{
			logic_reset();
			break;
		}

		BuildingBody *body = destination->building;
		BuildingBase *build = body->base;
		while (timerAction->next_surplus(context->get_time()))
		{
			Resources boolResource = this->rActionBool;
			build->updateInfo = true;
			bool transferSuccess = Resources::transfer(
				build->rStorage,
				rInventory,
				&weights,
				transferSize,
				rInventoryCap,
				inventorySize,
				boolResource);
			if (!transferSuccess)
			{
				/*DEBUG("BStore: %s, Inv: %s, Trsfr: %d, InvCap: %s, WeightLmt: %d, Bool: %s\n",
					  CSTR(build->rStorage),
					  CSTR(rInventory),
					  (int)transferSize,
					  CSTR(rInventoryCap),
					  (int)inventorySize,
					  CSTR(boolResource));*/
				logic_reset();
			}
		}
	}
	break;
	case Action::TRANSFER:
	{
		auto destination = pathData.destination;
		BuildingBody *body = destination->building;
		if (!body)
		{
			logic_reset();
			break;
		}

		BuildingBase *build = body->base;
		while (timerAction->next_surplus(context->get_time()))
		{
			Resources boolResource = this->rActionBool;

			build->updateInfo = true;

			Resources buildStorageBefore = build->rStorage;
			if (!Resources::transfer(
					rInventory,
					build->rStorage,
					&weights,
					transferSize,
					build->rStoreCap,
					build->weightCap,
					boolResource))
			{
				logic_reset();
			}
			context->record_resource_gain_delta(buildStorageBefore, build->rStorage);
		}
	}
	break;
	case Action::BUILD:
	{
		auto destination = pathData.destination;
		BuildingBody *body = destination->building;
		if (!body)
		{
			logic_reset();
			break;
		}
		BuildingBase *build = body->base;
		auto itr = context->constructions.find(build);
		if (itr == context->constructions.end())
		{
			logic_reset();
			break;
		}

		ConstructionData &data =
			context->constructions.at((*itr).second);
		while (timerAction->next_surplus(context->get_time()))
		{
			double x;
			x = workplace->props.num_get(
				PropertyNum::WORK_EFFICIENCY);
			data.work += math_min((int)x, data.work_left());
			data.work = math_max(data.work, 0);
			assert(x);
			build->updateInfo = true;
		}

		if (data.work_left() == 0)
		{
			ConstructionData tmp =
				ConstructionData(std::move(data));
			context->constructions.erase_value(build);
			context->delete_building(build);

			tmp.queueData.isConstruction = false;
			tmp.queueData.useResources = false;
			BuildingBase *newBuild = context->add_building(
				tmp.queueData, 0);
			if (context->infoBuild == build)
			{
				context->infoBuild = newBuild;
				newBuild->updateInfo = true;
			}

			logic_reset();
		}
	}
	break;
	case Action::ATTACKED:
		if (followers.empty())
			logic_reset();
		break;
	default:
		WARNING("Wrong action \"%d\" for building named \"%s\" of type \"%d\"", (int)action, workplace->name, (int)workplace->buildType);
		assert(false);
		break;
	}
}

// Logic Test01
void EntityCitizen::logic_test01()
{
	static auto resetActionTest01 = [](EntityCitizen *e)
	{
		e->reset();

		/*
		PathData pathData = find_building_and_path(
			e->context, e->pos,
			t_idpair{ ENUM_BODY_TYPE, (t_id)BodyType::BUILDING },
			-1.f,
			[](const sf::Vector2i &, const GameBody *, int) {
				//const BuildingBase *b = dynamic_cast<const BuildingBody *>(gb)->base;
				return true;// prng_get_double() > 0.75f;
			});
		e->set_path(std::move(pathData));
			*/

		auto b = add_request_find_building(
			*e->context->threadPath,
			e->context, e->pos,
			t_idpair{ENUM_BODY_TYPE, (t_id)BodyType::BUILDING},
			[](const sf::Vector2i &, const GameBody *, int)
			{
				return prng_get_double() > 0.75f;
			});

		delete e->buildFind;
		e->buildFind = new QueryThreadInstance<PathData>(
			std::move(b));
	};

	switch (action)
	{
	case Action::IDLE:
	{
		if (!buildFind)
		{
			while (timerPath->next_surplus(context->get_time()))
			{
				resetActionTest01(this);
			}
		}
		else if (buildFind->ready())
		{
			PathData pathData = buildFind->get();
			set_path(std::move(pathData));
			delete buildFind;
			buildFind = nullptr;
		}
	}
	break;
	case Action::MOVE:
	{
		auto destination = pathData.destination;
		assert(destination && "Moving without destination");
		if (!destination || !destination->building || !pathData.valid())
		{
			while (timerPath->next_surplus(context->get_time()))
				resetActionTest01(this);
			break;
		}

		auto &nearest = this->nearbyBuilds;
		auto itr = nearest.begin();
		assert(destination);

		// Check if the destination in one of the nearby buildings
		auto itrWorkplace = std::find(nearest.begin(), nearest.end(), destination->building);
		if (!destination->building ||
			(itr = itrWorkplace) == nearest.end())
		{
			break;
		}

		// Once a nearby building was detected
		BuildingBody *body = *itr;
		nearest.pop_front();
		BuildingBase *build = body->base;

		{
			resetActionTest01(this);
			timerAction->reset(context->get_time());
		}
	}

	break;

	default:
		break;
	}
}

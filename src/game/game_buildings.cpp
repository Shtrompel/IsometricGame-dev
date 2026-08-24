#include "game/game_buildings.hpp"

#include "game/game_data.hpp"

static std::vector<UpgradeTree *> recursive_get_available_tree(
	UpgradeTree *step,
	const std::vector<int> &upgrades)
{
	std::vector<UpgradeTree *> ret;

	if (!step)
		return ret;

	auto itr = std::find(
		upgrades.begin(),
		upgrades.end(),
		step->upgradeId);

	if (itr == upgrades.end())
	{
		ret.push_back(step);
	}
	else
	{
		auto &childs = step->children;
		for (auto itr = childs.begin(); itr != childs.end(); ++itr)
		{
			UpgradeTree *s = &*itr;
			merge_vectors(
				ret,
				recursive_get_available_tree(s, upgrades));
		}
	}

	return ret;
}

t_sprite UpgradeTree::get_sprite(
	const sf::Vector2i &pos,
	bool) const
{
	auto &sprites = this->spriteHolders;
	auto itr = sprites.find(pos);
	if (itr == sprites.end())
	{
		LOG_ERROR(
			"Sprite at index %s wasn't found in tree with id of %d",
			VEC_CSTR(pos),
			type.id);
		return -1;
	}
	else
		return (*itr).second;
}

std::string UpgradeTree::to_string(GameData* data, bool verbose) const {
	std::stringstream ss;

	if (verbose) {
		ss << "Name: " << (name[0] ? name : "Unknown") << "\n";
		if (description[0]) ss << "Description: " << description << "\n";
		if (hp != NULL_INT) ss << "HP: " << hp << "\n";
		if (buildWork > 0) ss << "Build Work: " << buildWork << "\n";
		
		if (data) {
			Resources b = build, i = input, o = output;
			if (!b.empty()) ss << "Cost: " << data->resources_to_str(b) << "\n";
			if (!i.empty()) ss << "Input: " << data->resources_to_str(i) << "\n";
			if (!o.empty()) ss << "Output: " << data->resources_to_str(o) << "\n";
		}
		
		if (powerIn > 0) ss << "Power In: " << powerIn << "\n";
		if (powerOut > 0) ss << "Power Out: " << powerOut << "\n";
		if (powerStore > 0) ss << "Power Store: " << powerStore << "\n";
		if (entityLimit != NULL_INT && entityLimit > 0) ss << "Entity Limit: " << entityLimit << "\n";
		if (effectRadius != NULL_FLOAT && effectRadius > 0) ss << "Effect Radius: " << effectRadius << "\n";
	} else {
		ss << (name[0] ? name : "Unknown");
		if (data) {
			Resources b = build;
			if (!b.empty()) ss << " (Cost: " << data->resources_to_str(b) << ")";
		}
	}

	return ss.str();
}

// Building base

BuildingBase::BuildingBase()
{
}

BuildingBase::BuildingBase(
	GameData *context,
	const sf::Vector2i &pos,
	UpgradeTree *step,
	const t_build_enum &type,
	size_t id)
	: Variant(
		  SERIALIZABLE_BUILD_BASE,
		  id)
{

	// todo, change timer length to stats value
	this->context = context;
	this->tilePos = pos;
	costTimer = context->create_timer(context->get_time());
	costTimer->set_length(COST_TIMER);
	costTimer->set_length(1.f);

	actionTimer = context->create_timer(context->get_time());

	this->tree = step;
	this->buildType = type;
	this->actionTimer->set_length(1.f);
	load_upgrade_step(step);
}

UpgradeTree *BuildingBase::get_tree(t_id id)
{
	GameBodyConfig *tree = context->bodyConfigMap.at(
		ENUM_BUILDING_TYPE, (t_id)info->buildType)[id];
	return dynamic_cast<UpgradeTree *>(tree);
}

void BuildingBase::variant_initialize()
{
	ASSERT_ERROR("%s \"variant_initialize\" should not be called.", __FUNCTION__);
}

json BuildingBase::to_json() const
{
	json ret;

#define j ret
	j["id"] = id;
	j["tilePos"] = tilePos;
	j["tilesSize"] = tilesSize;
	j["name"] = name;
	j["buildType"] = buildType;
	j["job"] = job;
	j["entitiesSpawned"] = entitiesSpawned;
	j["spawn"] = spawn;
	j["alignment"] = alignment;

	j["effectRadius"] = effectRadius;
	j["weightCap"] = weightCap;
	j["entityLimit"] = entityLimit;
	j["active"] = active;
	j["sufficient"] = sufficient;
	j["rIn"] = rIn;
	j["rOut"] = rOut;
	j["rStoreCap"] = rStoreCap;
	j["powerIn"] = powerIn;
	j["powerOut"] = powerOut;
	j["powerStore"] = powerStore;
	j["props"] = props;
	j["sprites"] = sprites;

	j["hp"] = hp;
	j["lastHp"] = lastHp;
	j["operational"] = operational;
	j["flagDelete"] = flagDelete;
	j["entities"] = entities;
	j["storedEntities"] = storedEntities;
	j["costTimer"] = costTimer;
	j["actionTimer"] = actionTimer;
	j["animFrame"] = animFrame;
	j["rStorage"] = rStorage;
	j["rPending"] = rPending;
	j["powerValue"] = powerValue;
	j["lastPowerOut"] = lastPowerOut;
	j["finalPowerOut"] = finalPowerOut;
	j["binded"] = binded;
	if (network)
	{
		j["network"] = network.to_json_ptr();
	}
	j["updateInfo"] = updateInfo;
	j["followers"] = followers;
	j["upgrades"] = upgrades;

#undef j

	return ret;
}

void BuildingBase::from_json(const json &j)
{
	// Force clear
	this->entities.clear();       
    this->storedEntities.clear(); 
    this->binded.clear();         
    this->followers.clear();      
    this->upgrades.clear();       

	try
	{
#define _GET(str) j.find(str).value()

		_GET("id").get_to(info->id);
		_GET("tilePos").get_to(info->tilePos);
		_GET("tilesSize").get_to(info->tilesSize);
		std::string strName;
		
		_GET("name").get_to(strName);
		strncpy(info->name, strName.c_str(), sizeof(info->name) - 1);
    	info->name[sizeof(info->name) - 1] = '\0';

		
		_GET("buildType").get_to(info->buildType);
		_GET("job").get_to(info->job);
		_GET("entitiesSpawned").get_to(info->entitiesSpawned);
		_GET("spawn").get_to(info->spawn);
		_GET("alignment").get_to(info->alignment);

		_GET("effectRadius").get_to(info->effectRadius);
		_GET("weightCap").get_to(info->weightCap);
		_GET("entityLimit").get_to(info->entityLimit);
		_GET("active").get_to(info->active);
		_GET("sufficient").get_to(info->sufficient);
		_GET("rIn").get_to(info->rIn);
		_GET("rOut").get_to(info->rOut);
		_GET("rStoreCap").get_to(info->rStoreCap);
		_GET("powerIn").get_to(info->powerIn);
		_GET("powerOut").get_to(info->powerOut);
		_GET("powerStore").get_to(info->powerStore);
		_GET("props").get_to(info->props);
		_GET("sprites").get_to(info->sprites);

		_GET("hp").get_to(data->hp);
		_GET("lastHp").get_to(data->lastHp);
		_GET("operational").get_to(data->operational);
		_GET("flagDelete").get_to(data->flagDelete);

		data->entities.clear();
		data->entities.shrink_to_fit();
		_GET("entities").get_to(data->entities);
		
		data->storedEntities.clear();
		data->storedEntities.shrink_to_fit();
		_GET("storedEntities").get_to(data->storedEntities);
		
		if (!costTimer.get())
			costTimer = t_time_manager::make_independent_timer();
		_GET("costTimer").get_to(data->costTimer);
		if (!actionTimer.get())
			actionTimer = t_time_manager::make_independent_timer();
		_GET("actionTimer").get_to(data->actionTimer);
		_GET("animFrame").get_to(data->animFrame);
		_GET("rStorage").get_to(data->rStorage);
		_GET("rPending").get_to(data->rPending);
		_GET("powerValue").get_to(data->powerValue);
		_GET("lastPowerOut").get_to(data->lastPowerOut);
		_GET("finalPowerOut").get_to(data->finalPowerOut);
		if (j.contains("binded"))
		{
			_GET("binded").get_to(data->binded);
		}
		if (j.contains("network"))
		{
			data->network.from_json_ptr(_GET("network"));
			// _GET("network").get_to(data->network);
		}
		_GET("updateInfo").get_to(data->updateInfo);
		if (j.contains("followers"))
		{
			_GET("followers").get_to(data->followers);
		}
		if (j.contains("upgrades"))
		{
			_GET("upgrades").get_to(data->upgrades);
		}

#undef _GET
	}
	catch (json::exception &e)
	{
		DEBUG("%s", j.dump().c_str());
		LOG_ERROR("Can't desirialize BuildingBase: %s", e.what());
	}
}

void BuildingBase::serialize_publish(const SerializeMap &map)
{
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);

	g->add_timer(costTimer);
	g->add_timer(actionTimer);

	this->tree = dynamic_cast<UpgradeTree *>(
		g->bodyConfigMap.at(
			ENUM_BUILDING_TYPE,
			(t_id)info->buildType)[0]);
	assert(this->tree);

	for (VariantPtr<GameBody>& b : followers)
		if (b.is_valid())
			map.apply(b);
	
	for (auto &v : binded)
		map.apply(v);
	if (network)
		map.apply(network);
	
	for (auto &e : entities)
		map.apply(e);
	for (auto &e : storedEntities)
		map.apply(e);
}

void BuildingBase::serialize_initialize(const SerializeMap &map)
{
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);

	g->confirm_building_base(this);
	// g->objectId = g->counterContext.objectIdCounter.advance<BuildingBase>();
}

std::string BuildingBase::to_string()
{
	std::stringstream s("");
	s << "\t\t\t" << get_format_name() << "\n";

	if (context->bodyConfigMap.count(
			ENUM_BUILDING_TYPE, (t_id)(info->buildType)))
	{
		auto tree = get_tree();
		s << "Description: " << tree->description << "\n";
	}

	s << "HP: " << hp << "\n";
	if (entityLimit > 0)
	{
		s << "Entities: " << entities.size();
		s << " / " << entityLimit;
		s << "\n";
	}
	if (!rIn.empty())
		s << "Input: " << rIn.to_string_weights(context->resourceContext.weights) << "\n";
	if (!rOut.empty())
		s << "Output: " << rOut.to_string_weights(context->resourceContext.weights) << "\n";

	bool a = !rStoreCap.is_infinity();
	bool b;
	b = weightCap > 0;
	if ((a || b) && !rStoreCap.empty())
	{
		s << "Storage: ";
		s << rStorage.to_string_weights(context->resourceContext.weights) << " ";
		s << "W: " << rStorage.weight(&context->resourceContext.weights);
		s << "/ ";
		if (a)
			s << rStoreCap.to_string_weights(context->resourceContext.weights);
		if (b)
			s << "W: " << weightCap;
		s << "\n";
	}

	if (network)
	{
		if (powerIn > 0)
			s << "Power In: " << powerIn << "\n";
		if (powerOut > 0)
			s << "Power Out: " << powerOut << "\n";
		if (powerStore > 0)
			s << "Power Out: " << powerStore << "\n";

		auto strs = str_split(network->to_string(), "}");
		std::string str = iterator_to_string(strs.begin(), strs.end(), "}\n");
		s << "Network: " << str;
	}

	return s.str();
}

std::string BuildingBase::get_format_str(const std::string &name)
{
	std::string str = string_format("", name);
	str_replace(str, "_", " ");
	return str;
}

std::string BuildingBase::get_format_name()
{
	std::string str = string_format("", name);
	str_replace(str, "_", " ");
	return str;
}

FVec BuildingBase::get_center_pos()
{
	return (FVec)tilePos + (FVec)tilesSize / 2.f;
}

void BuildingBase::load_upgrade_step(UpgradeTree *step, void *tree)
{
	if (!step)
		return;

	if (std::find(upgrades.begin(), upgrades.end(), step->upgradeId) != upgrades.end())
	{
		WARNING("Upgrade \"%d\" name: \"%s\" has already applied", step->upgradeId, step->name);
		return;
	}

	// Check if the building was powered prior to the upgrade
	bool wasPowered = props.bool_is(PropertyBool::POWER_NETWORK);

	this->id = step->type.id;
	this->upgrades.push_back(step->upgradeId);
	if (step->size != sf::Vector2i{-1, -1})
		this->tilesSize = step->size;

	if (strlen(step->name))
	{
		if (strlen(this->name))
		{
			DEBUG("Before name change %s -> \"%s\"", step->name, this->name);
			strcpy(
				this->name,
				string_format(this->name, step->name).c_str());
			DEBUG("After %s -> \"%s\"", step->name, this->name);
		}
		else
		{
			strcpy(
				this->name,
				step->name);
		}
	}

	if (step->hp != NULL_INT)
		this->lastHp = this->hp = step->hp;
	if (step->alignment != NULL_INT)
		this->alignment = step->alignment;
	// "isObstacle", "isBarrier"
	// Different names for the same variables?
	// Too bad!

	if (step->entityLimit != NULL_INT)
		this->entityLimit = step->entityLimit;
	if (step->delayCost != NULL_FLOAT)
		this->costTimer->set_length(step->delayCost);
	if (step->delayAction != NULL_FLOAT)
		this->actionTimer->set_length(step->delayAction);

	if (step->effectRadius != NULL_FLOAT)
		this->effectRadius = step->effectRadius;
	if (step->weightCap != NULL_INT)
		this->weightCap = step->weightCap;

	if (step->job != CitizenJob::NONE)
		this->job = (size_t)step->job;

#define SET_R(a, b) \
	if (!b.empty()) \
		a = b;

#define SET_I(a, b)    \
	if (b != NULL_INT) \
		a = b;

	SET_I(this->powerOut, step->powerOut);
	SET_I(this->powerIn, step->powerIn);
	SET_I(this->powerStore, step->powerStore);

	SET_R(this->rIn, step->input);
	SET_R(this->rOut, step->output);
	SET_R(this->rStoreCap, step->storageCap);

	if (step->spawn.serializableID != 0 &&
		step->spawn.id != IDPAIR_NONE)
		this->spawn = step->spawn;

	if (step->upgradeId == 0)
	{
		for (auto &[key, value] : step->spriteHolders)
			this->sprites[key] = value;
	}
	else
	{
		WARNING("Sprites upgrade uninplemented");
	}

	props.append(step->props);

	if (!wasPowered && props.bool_is(PropertyBool::POWER_NETWORK) && context)
	{
		context->connect_building_network(this);
	}
	
	for (auto& entityPtr : this->entities)
	{
		if (!entityPtr.is_valid())
			continue;;
		
		if (entityPtr->type != BodyType::ENTITY)
			continue;

		EntityCitizen* citizen = dynamic_cast<EntityCitizen*>(entityPtr.get());
		if (!citizen) 
			continue;
		
		citizen->recalculate_worker_stats();
	}
}

bool BuildingBase::tree_similar(BuildingBase *other)
{
	if (!other)
		return false;
	if (this == other)
		return true;
	if (buildType != other->buildType)
		return false;
	const auto &ua = this->upgrades;
	const auto &ub = other->upgrades;
	if (ua.size() != ub.size())
		return false;
	for (auto &a : ua)
	{
		bool found = false;
		for (auto &b : ub)
		{
			if (a != b)
				continue;
			found = true;
		}
		if (!found)
			return false;
	}
	return true;
}

void BuildingBase::damage(int attack)
{
	if (hp == BUILDING_INFINITE_HEALTH)
		return;
	hp -= attack;
	if (hp <= 0)
		hp = 0;
}

BuildingBody *BuildingBase::get_body() const
{
	Tile &t = context->chunks->get_tile(tilePos.x, tilePos.y);
	return t.building;
}

inline std::list<BuildingBody *> BuildingBase::get_bodies()
{
	std::list<BuildingBody *> ret;
	for (int x = 0; x < tilesSize.x; ++x)
	{
		for (int y = 0; y < tilesSize.y; ++y)
		{
			Tile &t = context->chunks->get_tile(tilePos.x + x, tilePos.y + y);
			ret.push_back(t.building);
		}
	}
	return ret;
}

void BuildingBase::update_sprites(int rotation)
{	/*
	for (BuildingBody* body : get_bodies())
	{
		body->spriteHolder = rand()%20;//body->get_sprite(rotation);
	}*/
	// uneeded
}

bool BuildingBase::is_any_storage() const
{
	bool out;
	out = props.bool_is((t_id)PropertyBool::ANY_STORAGE);
	assert(out == (props.bool_is((t_id)PropertyBool::STORAGE) ||
				   props.bool_is((t_id)PropertyBool::TMP_STORAGE)));
	return out;
}

inline bool BuildingBase::is_operational() const
{
	if (props.bool_is(PropertyBool::WORKPLACE) &&
		entityLimit > 0 &&
		storedEntities.size() == 0)
		return false;

	return active && sufficient;
}

inline void BuildingBase::flag_deletion()
{
	this->flagDelete = true;
}

std::vector<UpgradeTree *> BuildingBase::get_upgrades()
{
	std::vector<UpgradeTree *> ret;
	if (tree)
		ret = recursive_get_available_tree(tree, upgrades);
	return ret;
}

void BuildingBase::accept_entity(EntityCitizen *e)
{
	// DEBUG(
	//	"Size before: %d, limit: %d",
	//  	(int)entities.size(), (int)entityLimit);

	e->workplace = this;

	if (std::find(entities.begin(), entities.end(), e) == entities.end())
		this->entities.push_back(e);

	// Can not accept an entity that already has a job
	assert(e->job == CitizenJob::NONE);

	// If workplace's full, remove it from the "NEEDS_WORKERS" tree
	if (entities.size() >= entityLimit)
	{
		const static auto nw = (t_id)IngameProperties::NEEDS_WORKERS;

		const auto &bodies = get_bodies();
		std::for_each(bodies.cbegin(), bodies.cend(), [this](GameBody *body)
					  {
			BuildingBody* bbody = dynamic_cast<BuildingBody*>(body);
			context
				->get_tree(ENUM_INGAME_PROPERTIES, nw)
				->remove(bbody->tilePos, body); });
	}

	// DEBUG(
	//	"Size after: %d, limit: %d",
	//	(int)entities.size(), (int)entityLimit);
}

void BuildingBase::accept_entity_job(EntityCitizen *e)
{
	context
		->get_tree(ENUM_CITIZEN_JOB, (t_id)e->job)
		->remove(vec_pos_to_tile(e->pos), e);

	e->job = (CitizenJob)this->job;
	bool success;
	EntityStats *stats = context->get_entity_stats(
		ENUM_CITIZEN_JOB,
		this->job,
		&success);
	if (success)
	{
		e->apply_data(dynamic_cast<GameBodyConfig *>(stats));
		e->spriteHolder = e->spriteWalk;
	}

	// Insert the worker to his quad tree
	context
		->get_tree(ENUM_CITIZEN_JOB, (t_id)e->job)
		->insert(vec_pos_to_tile(e->pos), e);
	
	// Apply building specific stats
	e->recalculate_worker_stats();
}

void BuildingBase::enter_entity(EntityCitizen *e)
{
	if (this->props.bool_is(PropertyBool::INSIDE))
	{
		e->reset();
		e->insideWorkplace = true;
		this->storedEntities.push_back(e);
		auto v = vec_pos_to_tile(e->pos);
		ASSERT_ERROR(context->chunks->get_tile(v.x, v.y).remove_entity(e), "Failed attempt to remove entity from tile");
		context->hide_entity(e);
		e->action = EntityBody::Action::WORK;
		e->timerPath->reset(context->get_time());
		if (props.num_is(PropertyNum::ACTION_TIME))
		{
			double x;
			x = props.num_get(PropertyNum::ACTION_TIME);
			e->timerAction->set_length((float)x);
		}
	}
}

void BuildingBase::exit_entity(EntityCitizen *e)
{
	assert(e->workplace == this);
	if (this->props.bool_is(PropertyBool::INSIDE) &&
		e->workplace == this &&
		e->insideWorkplace)
	{
		e->reset();
		e->insideWorkplace = false;
		auto itr = std::find(
			storedEntities.begin(),
			storedEntities.end(),
			dynamic_cast<EntityBody *>(e));
		assert(itr != storedEntities.end());
		storedEntities.erase(itr);
		auto v = vec_pos_to_tile(e->pos);
		context->chunks->get_tile(v.x, v.y).entities.push_back(e);
		e->action = EntityBody::Action::IDLE;
		context->show_entity(e);
	}
}

// game_buildings.cpp (around line 850)
std::vector<VariantPtr<EntityBody>>::iterator
BuildingBase::remove_entity(EntityCitizen *e)
{
	assert(e->workplace == this);

	// If work space was created, add to the "NEEDS_WORKERS" tree
	if (entities.size() == entityLimit)
	{
		static const auto nw = (t_id)IngameProperties::NEEDS_WORKERS;
		const auto &bodies = get_bodies();
		std::for_each(bodies.cbegin(), bodies.cend(), [this](GameBody *body)
					  {
			// Don't add a building if it has a null body
			// The quad trees should never hold null values
			if (!body) 
				return;

			BuildingBody* bbody = dynamic_cast<BuildingBody*>(body);
			context->get_tree(ENUM_INGAME_PROPERTIES, nw)->insert(bbody->tilePos, body); });
	}

	t_tiletree* jobTree;
	context->get_tree(ENUM_CITIZEN_JOB, (t_id)e->job);

	// Remove from the old job tree BEFORE changing the variable
	jobTree->remove(vec_pos_to_tile(e->pos), e);

	e->workplace = nullptr;
	e->job = CitizenJob::NONE;

	auto itr = std::find(entities.begin(), entities.end(), e);
	assert(itr != entities.end());
	auto ret = this->entities.erase(itr);

	// Add it to the new NONE tree so it can be properly tracked
	jobTree->insert(vec_pos_to_tile(e->pos), e);

	return ret;
}

void BuildingBase::update()
{
	// Here's goes the stuff the can happen even if
	// the building is inactive.

	// If the building is a depleted raw resource, hide it and mark it
	// for later deletion .
	if (props.bool_is(PropertyBool::HARVESTABLE) &&
		rStorage.empty())
	{
		this->flagDelete = true;
		this->hp = 0;
		auto bodies = get_bodies();
		std::for_each(bodies.begin(), bodies.end(),
					  [](GameBody *body)
					  { body->visible = false; });
	}

	int actionTimes = actionTimer->count_times(context->get_time());

	if (this->hp != this->lastHp)
	{
		this->updateInfo = true;
		this->lastHp = this->hp;
	}

	if (this->hp <= 0 && this->hp != BUILDING_INFINITE_HEALTH)
	{
		this->flagDelete = true;
	}

	// Go through every propertyVals
	for (const auto &pPair : props.nums_itr())
	{
		switch (pPair.first)
		{
			// Removes a percentage amount of resources
			// BAD FEATURE, REMOVE
		case PropertyNum::DECAY_AMOUNT:
		{
			break;
			while (actionTimer->next_surplus(context->get_time()))
			{
				Resources x = this->rStorage;
				x *= (float)pPair.second;
				this->rStorage = x;
			}
			break;
		}
		// Change frames according to the storage capacity
		case PropertyNum::STORAGE_FRAMES:
		case PropertyNum::POWER_FRAMES:
		case PropertyNum::ENTITY_FRAMES:
		{
			auto bodies = get_bodies();
			double storageFrames = pPair.second;
			std::for_each(
				bodies.begin(),
				bodies.end(),
				[this, pPair, storageFrames](BuildingBody *body)
				{
					float frame = 0;
					if (pPair.first == PropertyNum::POWER_FRAMES)
					{
						frame = (float)powerValue / powerStore;
					}
					else if (pPair.first == PropertyNum::ENTITY_FRAMES)
					{
						frame = (float)storedEntities.size() / (entityLimit + 1);
					}
					else if (weightCap > 0)
						frame = (float)rStorage.weight(&context->resourceContext.weights) / weightCap;
					else
					{
						frame = rStorage.get_presentage_of(rStoreCap);
					}

					frame *= (float)storageFrames;
					frame = math_min<float>(frame, (float)storageFrames);

					body->animFrame = (int)frame;

					if (0 && context->frameCount % 50 == 0)
					{
						DEBUG("%s %s %f %f %f %d",
							  CSTR(rStorage),
							  CSTR(rStoreCap),
							  weightCap,
							  rStorage.get_presentage_of(rStoreCap),
							  (float)storageFrames,
							  (int)body->animFrame);
					}
				});
		}
		break;
		default:
			continue;
		}
	}

	// From here, everything can only work if the building is active
	if (!active)
		return;

	// If the building is a workplace, multiply every resource
	// related logic by the workers count.
	int storedCount = props.bool_is(PropertyBool::WORKPLACE)
						  ? (int)storedEntities.size()
						  : 1;

	bool wasSufficient = sufficient;
	unsigned ioTime = 0;

	// Resource io goes here

	// Count how many time we should consume resources
	while (costTimer->next_surplus(context->get_time()))
		++ioTime;

	// If it's time to gemerate and consume resources
	for (unsigned i = 0; i < ioTime; ++i)
	{
		// Check if storage can afford the input
		bool hasResources = rStorage.affordable(rIn * storedCount);

		// Avoid resource consumption if there is no power
		bool hasPower = !(powerIn > 0 && !wasSufficient);
		sufficient = hasResources && hasPower;

		// If it's part of a network
		if (network &&
			props.bool_is(PropertyBool::POWER_NETWORK) &&
			this->powerOut)
		{
			// If the building had the ability to generate power
			if (wasSufficient && !sufficient)
			{
				// Slowly dampen the power it's generate until it's 0s
				network->powerOut -= this->lastPowerOut;
				this->finalPowerOut = 0;
				this->updateInfo = true;
			}
			else if (!wasSufficient && sufficient)
			{
			}
		}
	}

	// Unable to operate, stop
	if (!sufficient)
		return;

	// Reset pending resources, remove it for keeping them
	// todo remove?
	rPending = Resources::res_empty(&context->resourceContext.weights);

	for (unsigned i = 0; i < ioTime; ++i)
	{
		Resources out = this->rPending;

		if (buildType == (int)BuildingType::GENERATOR && 0)
			DEBUG("%s %s %d %s", CSTR(rStorage), CSTR(this->rIn), (int)storedCount, CSTR((this->rIn * storedCount)));
		// Remove from storage the nessesey resources
		rStorage -= this->rIn * storedCount;

		// If the building had the ability to geenrate power
		if (network &&
			props.bool_is(PropertyBool::POWER_NETWORK) &&
			powerOut)
		{
			// Slowly accelerate the power output until the desired amoint
			int powerOut2 = this->powerOut * storedCount;
			int newOut = this->finalPowerOut;
			int dif = math_sign(powerOut2 - finalPowerOut);
			newOut += dif * POWER_SPEED;
			newOut = dif > 0
						 ? math_min(newOut, powerOut2)
						 : math_max(newOut, powerOut2);

			network->powerOut += newOut;
			network->powerOut -= this->finalPowerOut;

			this->finalPowerOut = newOut;

			this->updateInfo = true;
		}

		// Find how many resources need to be generated
		out += this->rOut * storedCount;

		// If something was generated
		if (!out.empty())
		{
			// Move the resources to the building's storage
			Resources::transfer(
				out,
				rStorage,
				&context->resourceContext.weights,
				weightCap,	// Max transfer amount
				rStoreCap,	// Max storage capacity
				weightCap); // Transfer count
			this->updateInfo = true;
		}
	}

	bool isOffensive = props.bool_is(PropertyBool::OFFENSIVE);
	bool isHome = props.bool_is(PropertyBool::HOME);

	GameBody *target = nullptr;

	if (isOffensive)
	{
		float effectRadius = this->effectRadius;
		auto entities =
			context->nearest_bodies_quad(
				this->get_center_pos(),
				1ull,
				{ENUM_ALIGNMENT, GameData::alignment_opposite(this->alignment)},
				[effectRadius](const sf::Vector2i &pos,
							   const GameBody *gb,
							   const int dist)
				{
					return !gb->dead && dist < effectRadius;
				});

		if (entities.size())
		{
			target = dynamic_cast<GameBody *>(entities.front());
		}
	}

	//if (!target && !isHome && spawn.serializableID != 0)
	//	return;

	// Spawning
	for (int i = 0; i < actionTimes; ++i)
	{

		if (!is_operational())
			continue;

		// Clean up dead entities (null VariantPtrs) so they don't count towards the limit
		for (auto itr = entities.begin(); itr != entities.end();) {
			if (itr->is_null())
				itr = entities.erase(itr);
			else
				++itr;
		}

		if (entities.size() >= entityLimit)
		{
			continue;
		}

		if (spawn.id.group != ENUM_NONE &&
			spawn.serializableID != 0)
		{
			sf::Vector2f freePos;
			BodyQueueData data{};

			if (isHome)
			{
				if (!context->get_free_neighbor(
						freePos, this->get_bodies().back()->pos))
					continue;
			}
			else
				freePos = get_center_pos();

			data.spawn = this->spawn;
			data.pos = freePos;
			data.alignment = this->alignment;

			if (isOffensive && target)
			{
				FVec targetPos = target->pos;
				if (target->type == BodyType::BUILDING)
				{
					BuildingBody *build;
					build = dynamic_cast<BuildingBody *>(target);
					targetPos = build->base->get_center_pos();
				}
				FVec dif = -(this->get_center_pos() - target->pos);
				vec_normalize(dif);

				if (dif != FVec{0.f, 0.f})
				{
					data.vel = dif;
					data.target = target;
				}
			}
			else
			{
				data.home = this;
			}
			
			DEBUG("Adding body: %s", data.to_string().c_str());

			context->queue_add_body(data);
		}
	}
}

// Building Body

BuildingBody::BuildingBody(
	GameData *context,
	const sf::Vector2i &pos,
	BuildingBase *base,
	t_sprite spriteHolder,
	sf::Vector2i spriteIndex,
	size_t id)
	: GameBody(
		  context,
		  SERIALIZABLE_BUILD_BODY,
		  id,
		  (sf::Vector2f)pos + sf::Vector2f{.5f, .5f}, // + (sf::Vector2f)base->tilesSize / 2.f,
		  -1,
		  BodyType::BUILDING,
		  sf::Vector2f{1.f, 1.f}),
	  tilePos(pos),
	  spriteHolder(spriteHolder),
	  spriteIndex(spriteIndex),
	  base(base)
{
	this->shape = ShapeType::RECT;
	this->frame = (int)base->animFrame;
	this->alignment = base->alignment;
}

BuildingBody::~BuildingBody()
{
}

int BuildingBody::get_hp() const
{
	if (dead)
		return 0;

	return base.get()->hp;
}

int &BuildingBody::get_hp()
{
	return base.get()->hp;
}

void BuildingBody::apply_data(GameBodyConfig *config)
{
	LOG_ERROR("Can't apply data to building body, should be applied to base");
	assert(0);
}

bool BuildingBody::confirm_body(GameData *context)
{
	GameBody::confirm_body(context);

	BuildingBody *body = this;
	body->context = context;

	Tile &tile = context->chunks->get_tile(body->tilePos.x, body->tilePos.y);
	tile.building = body;
	context->bodiesContext.bodies.push_back(body);

	context->bodiesContext.buildings.push_back(body);

	// Insert building body to quad trees
	auto trees = context->get_trees(dynamic_cast<GameBody *>(body));
	for (auto &pair : trees)
	{
		GameBody *b = dynamic_cast<GameBody *>(body);
		t_tiletree *tree = pair.second;
		assert(b);
		assert(tree && tile.building);
		if (!tree || !tile.building)
			continue;
		bool bb = tree->insert(body->tilePos, b);
		if (!bb)
		{
			ASSERT_ERROR(
				bb,
				"QuadTree insert failed");
			return false;
		}
	}

	return true;
}

t_sprite BuildingBody::get_sprite(int rotation) const
{
	if (spriteHolder == -1)
		return -1;
	sf::Vector2i axis = vec_90_rotate_axis<int>(-rotation);
	sf::Vector2i index = spriteIndex;
	if (axis.x == -1)
		index.x = base->tilesSize.x - index.x - 1;
	if (axis.y == -1)
		index.y = base->tilesSize.y - index.y - 1;
	auto itr = base->sprites.find(index);
	if (itr == base->sprites.end())
		itr = base->sprites.find({0, 0});
	assert(itr != base->sprites.end());
	return (*itr).second; // base->sprites[index];
}

json BuildingBody::to_json() const
{
	json j = GameBody::to_json();
	try
	{
			json buildArr = json::array();
		buildArr.push_back(tilePos);
		buildArr.push_back(start);
		buildArr.push_back(end);
		buildArr.push_back(spriteHolder);
		buildArr.push_back(frame);
		buildArr.push_back(maxFrame);
		buildArr.push_back(spriteIndex);
		buildArr.push_back(base.to_json_ptr());
		
		j["build"] = buildArr;
	}
	catch(const std::exception& e)
	{
		LOG_ERROR("Can't convert to json BuildingBody: %s", e.what());
	}

	
	return j;
}

void BuildingBody::from_json(const json &j0)
{
	try
	{
		GameBody::from_json(j0);
		json j = j0.at("build");

		tilePos = j[0].get<sf::Vector2i>();
		start = j[1].get<sf::Vector2i>();
		end = j[2].get<sf::Vector2i>();
		spriteHolder = j[3].get<t_sprite>();
		frame = j[4].get<int>();
		maxFrame = j[5].get<int>();
		spriteIndex = j[6].get<sf::Vector2i>();
		base.from_json_ptr(j[7]);
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize BuildingBody: %s", e.what());
	}
}

void BuildingBody::serialize_publish(const SerializeMap &map)
{
	GameBody::serialize_publish(map);

	map.apply(this->base);
	assert(this->base);
}

void BuildingBody::serialize_initialize(const SerializeMap &map)
{
	GameBody::serialize_initialize(map);

	// Get the context of the game
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);

	// Confirm the body
	this->confirm_body(g);
	// g->objectId = g->counterContext.objectIdCounter.advance<BuildingBody>();
}
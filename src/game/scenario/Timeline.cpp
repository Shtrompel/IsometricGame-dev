#include "game/scenario/timeline.hpp"

#include "game/game_data.hpp"

extern "C"
{
#include "libs/prng.h"
}



// ---------------------------------------------------------------------
// TutorialBlocking
// ---------------------------------------------------------------------

void to_json(json &j, const TutorialBlocking &v)
{
	j["lockGui"] = v.lockGui;
	j["toIgnore"] = v.toIgnore;
	j["lockMap"] = v.lockMap;
	j["lockGame"] = v.lockGame;
	j["doHighlightPos"] = v.doHighlightPos;
	j["highlightPos"] = v.highlightPos;
	j["highlightObjects"] = v.highlightObjects;
}

void from_json(const json &j, TutorialBlocking &v)
{
	v.lockGui = j.value("lockGui", false);
	v.toIgnore = j.value("toIgnore", std::vector<std::string>{});
	v.lockMap = j.value("lockMap", false);
	v.lockGame = j.value("lockGame", false);
	v.doHighlightPos = j.value("doHighlightPos", false);
	v.highlightPos = j.value("highlightPos", IVec{});
	v.highlightObjects = j.value("highlightObjects", std::vector<t_id>{});
}

// ---------------------------------------------------------------------
// DialogSequence
// ---------------------------------------------------------------------

void to_json(json &j, const DialogSequence::DialogActor &v)
{
	j["spriteHolder"] = v.spriteHolder;
	j["scale"] = v.scale;
	j["xPosition"] = v.xPosition;
	j["synthID"] = v.synthID;
}

void from_json(const json &j, DialogSequence::DialogActor &v)
{
	v.spriteHolder = j.value("spriteHolder", std::vector<t_sprite>{});
	v.scale = j.value("scale", FVec{1.f, 1.f});
	v.xPosition = j.value("xPosition", 0.5f);
	v.synthID = j.value("synthID", -1);
}

void to_json(json &j, const DialogSequence::DialogLine &v)
{
	j["actorIndex"] = v.actorIndex;
	j["transition"] = v.transition;
	j["text"] = v.text;
}

void from_json(const json &j, DialogSequence::DialogLine &v)
{
	v.actorIndex = j.value("actorIndex", -1);
	v.transition = j.value("transition", DialogSequence::Transition::IMMEDIATE);
	v.text = j.value("text", std::string());
}

void to_json(json &j, const DialogSequence &v)
{
	j["dialogLines"] = v.dialogLines;
	j["currentLine"] = v.currentLine;
}

void from_json(const json &j, DialogSequence &v)
{
	v.dialogLines = j.value("dialogLines", std::vector<DialogSequence::DialogLine>{});
	v.currentLine = j.value("currentLine", (size_t)0);
}

// ---------------------------------------------------------------------
// TimelineSpawnInfo
// ---------------------------------------------------------------------

void to_json(json &j, const TimelineSpawnInfo::PortalInstance &v)
{
	j["portalId"] = v.portalId;
	j["enable"] = v.enable;
	j["spawnOrigin"] = v.spawnOrigin;
	j["storedEntityLimit"] = v.storedEntityLimit;
	j["totalEntityLimit"] = v.totalEntityLimit;
	j["spawnInterval"] = v.spawnInterval;
	j["spawnPool"] = v.spawnPool;
	j["spawnType"] = v.spawnType;
	j["poolIndex"] = v.poolIndex;
	j["totalSpawned"] = v.totalSpawned;
	j["nextSpawnAt"] = v.nextSpawnAt;
}

void from_json(const json &j, TimelineSpawnInfo::PortalInstance &v)
{
	v.portalId = j.value("portalId", t_id{});
	v.enable = j.value("enable", true);
	v.spawnOrigin = j.value("spawnOrigin", IVec{});
	v.storedEntityLimit = j.value("storedEntityLimit", -1);
	v.totalEntityLimit = j.value("totalEntityLimit", -1);
	v.spawnInterval = j.value("spawnInterval", 0.f);
	v.spawnPool = j.value("spawnPool", std::vector<SpawnInfo>{});
	v.spawnType = j.value("spawnType", TimelineSpawnInfo::SpawnType::LINEAR);
	v.poolIndex = j.value("poolIndex", (size_t)0);
	v.totalSpawned = j.value("totalSpawned", 0);
	v.nextSpawnAt = j.value("nextSpawnAt", 0.f);
}

void to_json(json &j, const TimelineSpawnInfo &v)
{
	j["spawns"] = v.spawns;
}

void from_json(const json &j, TimelineSpawnInfo &v)
{
	v.spawns = j.value("spawns", std::vector<TimelineSpawnInfo::PortalInstance>{});
}

// ---------------------------------------------------------------------
// Timeline::TimelineEvent and its subclasses
// ---------------------------------------------------------------------

json Timeline::TimelineEvent::to_json() const
{
	json j;
	j["type"] = type;
	j["intro"] = intro;
	j["outro"] = outro;
	j["spawnInfo"] = spawnInfo;
	j["gameLocking"] = gameLocking;
	j["jumpCamera"] = jumpCamera;
	j["jumpTo"] = jumpTo;
	j["maxTime"] = maxTime;
	j["killAll"] = killAll;
	j["showEvent"] = showEvent;
	j["phase"] = phase;
	return j;
}

void Timeline::TimelineEvent::from_json(const json &j)
{
	type = j.value("type", MissionType::NONE);
	intro = j.value("intro", DialogSequence{});
	outro = j.value("outro", DialogSequence{});
	spawnInfo = j.value("spawnInfo", TimelineSpawnInfo{});
	gameLocking = j.value("gameLocking", TutorialBlocking{});
	jumpCamera = j.value("jumpCamera", false);
	jumpTo = j.value("jumpTo", IVec{});
	maxTime = j.value("maxTime", -1.f);
	killAll = j.value("killAll", false);
	showEvent = j.value("showEvent", true);
	phase = j.value("phase", Phase::INTRO_DIALOG);
}

bool Timeline::TimelineEvent::update(GameData *data, int timePassed)
{
	if (!data)
		return false;

	for (TimelineSpawnInfo::PortalInstance &portal : spawnInfo.spawns)
	{
		BuildingBase *building = data->find_building_by_id(portal.portalId);
		bool resolvedById = building != nullptr;
		if (!building)
			building = data->find_building_by_pos(portal.spawnOrigin);

		// Code made by Claude Sonnet 5 - spawnEnabled only gates the
		// building's own native upgrade-tree spawn loop, a separate
		// mechanism from spawnPool below. A portal with a spawnPool is
		// meant to be fully timeline-controlled, so its native loop must
		// stay off regardless of portal.enable - otherwise "enable" rearms
		// both mechanisms at once and the building fires its single
		// buildings.json-configured spawn alongside the pool
		if (building)
			building->spawnEnabled = portal.enable && portal.spawnPool.empty();

		// Code made by Claude Sonnet 5 - the timeline's own spawner, fully
		// independent of the building's upgrade-tree spawn loop (which
		// only ever uses spawnEnabled to gate itself, nothing else here).
		// Runs on this portal's own spawnInterval/pool rather than
		// whatever cadence the building would otherwise spawn on
		if (!portal.enable || portal.spawnPool.empty())
			continue;

		if ((t_seconds)timePassed < portal.nextSpawnAt)
			continue;

		if (portal.totalEntityLimit != -1 &&
			portal.totalSpawned >= portal.totalEntityLimit)
			continue;

		const SpawnInfo *next = nullptr;
		switch (portal.spawnType)
		{
		case TimelineSpawnInfo::SpawnType::RANDOM:
			next = &portal.spawnPool[(size_t)(prng_get_double() * portal.spawnPool.size())];
			break;
		case TimelineSpawnInfo::SpawnType::IGNORE:
			next = &portal.spawnPool[0];
			break;
		case TimelineSpawnInfo::SpawnType::LINEAR:
		default:
			next = &portal.spawnPool[portal.poolIndex % portal.spawnPool.size()];
			++portal.poolIndex;
			break;
		}

		// Code made by Claude Sonnet 5 - a building resolved by id spawns
		// from its actual position; spawnOrigin is only a fallback for
		// when no matching building exists in the world at all
		FVec spawnPos = resolvedById
			? building->get_center_pos()
			: FVec{(float)portal.spawnOrigin.x + 0.5f, (float)portal.spawnOrigin.y + 0.5f};

		BodyQueueData bqd{};
		bqd.spawn = *next;
		bqd.pos = spawnPos;
		bqd.alignment = ALIGNMENT_ENEMY;
		data->queue_add_body(bqd);

		++portal.totalSpawned;
		portal.nextSpawnAt = (t_seconds)timePassed + portal.spawnInterval;
	}

	return false;
}

Timeline::TimelineEvent *Timeline::TimelineEvent::create(MissionType type)
{
	switch (type)
	{
	case MissionType::SURVIVE_TIME:
		return new TimelineEventSurvive();
	case MissionType::SURVIVE_KILLS:
		return new TimelineEventKill();
	case MissionType::GAIN_RESOURCES:
		return new TimelineGainResources();
	case MissionType::BUILD_BUILDINGS:
		return new TimelineGainBuild();
	case MissionType::GAIN_POPULATION:
		return new TimelineGainPopulation();
	case MissionType::NONE:
	default:
		return new TimelineEvent();
	}
}

json Timeline::TimelineEventSurvive::to_json() const
{
	json j = TimelineEvent::to_json();
	j["missionTime"] = missionTime;
	return j;
}

void Timeline::TimelineEventSurvive::from_json(const json &j)
{
	TimelineEvent::from_json(j);
	missionTime = j.value("missionTime", -1);
}

bool Timeline::TimelineEventSurvive::init(GameData * gd)
{
    return false;
}

bool Timeline::TimelineEventSurvive::is_done(GameData* gameData, int timePassed)
{
	if (missionTime == -1)
		return false;

	if (timePassed > missionTime)
		return true;

    return false;
}

std::string Timeline::TimelineEventSurvive::get_status_text(GameData *, int timePassed) const
{
	if (missionTime == -1)
		return "";

	int left = missionTime - timePassed;
	if (left < 0)
		left = 0;

	return std::to_string(left) + " seconds left";
}

json Timeline::TimelineEventKill::to_json() const
{
	json j = TimelineEvent::to_json();
	j["maxKills"] = maxKills;
	j["maxKillPerType"] = maxKillPerType;
	j["killBaseline"] = killBaseline;
	j["killTypeBaseline"] = killTypeBaseline;
	return j;
}

void Timeline::TimelineEventKill::from_json(const json &j)
{
	TimelineEvent::from_json(j);
	maxKills = j.value("maxKills", -1);
	maxKillPerType = j.value("maxKillPerType", std::map<IdPair, int>{});
	killBaseline = j.value("killBaseline", 0L);
	killTypeBaseline = j.value("killTypeBaseline", std::map<IdPair, long>{});
}

bool Timeline::TimelineEventKill::init(GameData *data)
{
	if (!data)
		return false;

	killBaseline = data->get_deaths({ENUM_ENTITY_TYPE, (t_id)EntityType::ENEMY});
	for (const auto &entry : maxKillPerType)
		killTypeBaseline[entry.first] = data->get_deaths(entry.first);

	return false;
}

bool Timeline::TimelineEventKill::is_done(GameData *data, int)
{
	if (!data)
		return false;

	if (maxKills != -1)
	{
		long killed = data->get_deaths({ENUM_ENTITY_TYPE, (t_id)EntityType::ENEMY}) - killBaseline;
		if (killed < maxKills)
			return false;
	}

	for (const auto &entry : maxKillPerType)
	{
		long killed = data->get_deaths(entry.first) - killTypeBaseline[entry.first];
		if (killed < entry.second)
			return false;
	}

	return maxKills != -1 || !maxKillPerType.empty();
}

std::string Timeline::TimelineEventKill::get_status_text(GameData *data, int) const
{
	if (!data || maxKills == -1)
		return "";

	long killed = data->get_deaths({ENUM_ENTITY_TYPE, (t_id)EntityType::ENEMY}) - killBaseline;
	if (killed < 0)
		killed = 0;
	if (killed > maxKills)
		killed = maxKills;

	return std::to_string(killed) + " of " + std::to_string(maxKills) + " killed";
}

json Timeline::TimelineGainResources::to_json() const
{
	json j = TimelineEvent::to_json();
	j["resourcesGain"] = resourcesGain;
	j["resourcesStored"] = resourcesStored;
	j["resourcesBaseline"] = resourcesBaseline;
	return j;
}

void Timeline::TimelineGainResources::from_json(const json &j)
{
	TimelineEvent::from_json(j);
	resourcesGain = j.value("resourcesGain", std::map<int, int>{});
	resourcesStored = j.value("resourcesStored", std::map<int, int>{});
	resourcesBaseline = j.value("resourcesBaseline", std::map<int, long>{});
}

bool Timeline::TimelineGainResources::init(GameData *data)
{
	if (!data)
		return false;

	for (const auto &entry : resourcesGain)
		resourcesBaseline[entry.first] = data->get_resource_gained(entry.first);

	return false;
}

bool Timeline::TimelineGainResources::is_done(GameData *data, int)
{
	if (!data)
		return false;

	if (resourcesGain.empty() && resourcesStored.empty())
		return false;

	for (const auto &entry : resourcesGain)
	{
		long gained = data->get_resource_gained(entry.first) - resourcesBaseline[entry.first];
		if (gained < entry.second)
			return false;
	}

	for (const auto &entry : resourcesStored)
	{
		if (data->resourceContext.resources.get(entry.first) < entry.second)
			return false;
	}

	return true;
}

std::string Timeline::TimelineGainResources::get_status_text(GameData *data, int) const
{
	if (!data)
		return "";

	std::string out;
	for (const auto &entry : resourcesGain)
	{
		auto baselineItr = resourcesBaseline.find(entry.first);
		long baseline = baselineItr != resourcesBaseline.end() ? baselineItr->second : 0;
		long gained = data->get_resource_gained(entry.first) - baseline;
		if (gained < 0)
			gained = 0;
		if (gained > entry.second)
			gained = entry.second;

		if (!out.empty())
			out += " ";
		out += data->resourceContext.weights.get_str(entry.first) + " "
			+ std::to_string(gained) + " of " + std::to_string(entry.second);
	}

	for (const auto &entry : resourcesStored)
	{
		int current = data->resourceContext.resources.get(entry.first);
		if (current < 0)
			current = 0;
		if (current > entry.second)
			current = entry.second;

		if (!out.empty())
			out += " ";
		out += data->resourceContext.weights.get_str(entry.first) + " "
			+ std::to_string(current) + " of " + std::to_string(entry.second);
	}

	return out;
}

json Timeline::TimelineGainBuild::to_json() const
{
	json j = TimelineEvent::to_json();
	j["buildingCounter"] = buildingCounter;
	return j;
}

void Timeline::TimelineGainBuild::from_json(const json &j)
{
	TimelineEvent::from_json(j);
	buildingCounter = j.value("buildingCounter", std::map<t_id, int>{});
}

bool Timeline::TimelineGainBuild::is_done(GameData *data, int)
{
	if (!data || buildingCounter.empty())
		return false;

	std::map<t_id, int> counts;
	for (BuildingBase *b : data->bodiesContext.buildingBases)
		if (b)
			++counts[b->buildType];

	for (const auto &entry : buildingCounter)
		if (counts[entry.first] < entry.second)
			return false;

	return true;
}

std::string Timeline::TimelineGainBuild::get_status_text(GameData *data, int) const
{
	if (!data)
		return "";

	std::map<t_id, int> counts;
	for (BuildingBase *b : data->bodiesContext.buildingBases)
		if (b)
			++counts[b->buildType];

	std::string out;
	for (const auto &entry : buildingCounter)
	{
		int current = counts[entry.first];
		if (current > entry.second)
			current = entry.second;

		if (!out.empty())
			out += " ";
		out += data->enum_get_str(ENUM_BUILDING_TYPE, entry.first, true) + " "
			+ std::to_string(current) + " of " + std::to_string(entry.second);
	}

	return out;
}

json Timeline::TimelineGainPopulation::to_json() const
{
	json j = TimelineEvent::to_json();
	j["entitiyTypeSpawn"] = entitiyTypeSpawn;
	j["entitiyTypeTotal"] = entitiyTypeTotal;
	j["entitiesSpawn"] = entitiesSpawn;
	j["entitiesPopulation"] = entitiesPopulation;
	j["citizenBaseline"] = citizenBaseline;
	j["typeBaseline"] = typeBaseline;
	return j;
}

void Timeline::TimelineGainPopulation::from_json(const json &j)
{
	TimelineEvent::from_json(j);
	entitiyTypeSpawn = j.value("entitiyTypeSpawn", std::map<IdPair, int>{});
	entitiyTypeTotal = j.value("entitiyTypeTotal", std::map<IdPair, int>{});
	entitiesSpawn = j.value("entitiesSpawn", int{});
	entitiesPopulation = j.value("entitiesPopulation", int{});
	citizenBaseline = j.value("citizenBaseline", 0L);
	typeBaseline = j.value("typeBaseline", std::map<IdPair, long>{});
}

bool Timeline::TimelineGainPopulation::init(GameData *data)
{
	if (!data)
		return false;

	citizenBaseline = data->get_creations({ENUM_ENTITY_TYPE, (t_id)EntityType::CITIZEN});
	for (const auto &entry : entitiyTypeSpawn)
		typeBaseline[entry.first] = data->get_creations(entry.first);

	return false;
}

bool Timeline::TimelineGainPopulation::is_done(GameData *data, int)
{
	if (!data)
		return false;

	if (entitiesSpawn != -1)
	{
		long spawned = data->get_creations({ENUM_ENTITY_TYPE, (t_id)EntityType::CITIZEN}) - citizenBaseline;
		if (spawned < entitiesSpawn)
			return false;
	}

	for (const auto &entry : entitiyTypeSpawn)
	{
		long spawned = data->get_creations(entry.first) - typeBaseline[entry.first];
		if (spawned < entry.second)
			return false;
	}

	if (entitiesPopulation != -1 &&
		(int)data->bodiesContext.entityCitizens.size() < entitiesPopulation)
		return false;

	if (!entitiyTypeTotal.empty())
	{
		std::map<IdPair, int> counts;
		for (const VariantPtr<EntityCitizen> &c : data->bodiesContext.entityCitizens)
			if (c)
				++counts[{ENUM_CITIZEN_JOB, (t_id)c->job}];

		for (const auto &entry : entitiyTypeTotal)
			if (counts[entry.first] < entry.second)
				return false;
	}

	return entitiesSpawn != -1 || !entitiyTypeSpawn.empty() ||
		   entitiesPopulation != -1 || !entitiyTypeTotal.empty();
}

std::string Timeline::TimelineGainPopulation::get_status_text(GameData *data, int) const
{
	if (!data)
		return "";

	std::string out;

	if (entitiesPopulation != -1)
	{
		int current = (int)data->bodiesContext.entityCitizens.size();
		if (current > entitiesPopulation)
			current = entitiesPopulation;
		out += "Population " + std::to_string(current) + " of " + std::to_string(entitiesPopulation);
	}

	if (entitiesSpawn != -1)
	{
		long spawned = data->get_creations({ENUM_ENTITY_TYPE, (t_id)EntityType::CITIZEN}) - citizenBaseline;
		if (spawned < 0)
			spawned = 0;
		if (spawned > entitiesSpawn)
			spawned = entitiesSpawn;

		if (!out.empty())
			out += " ";
		out += "Spawned " + std::to_string(spawned) + " of " + std::to_string(entitiesSpawn);
	}

	return out;
}

// Rebuilds the correct polymorphic subclass from its "type" field.
static Timeline::TimelineEvent *timeline_event_from_json(const json &j)
{
	Timeline::MissionType type = j.value("type", Timeline::MissionType::NONE);
	Timeline::TimelineEvent *event = Timeline::TimelineEvent::create(type);
	event->from_json(j);
	return event;
}

// ---------------------------------------------------------------------
// Timeline
// ---------------------------------------------------------------------

json Timeline::to_json() const
{
	json j;
	j["actors"] = actors;
	j["isFinished"] = isFinished;

	json jPending = json::array();
	for (TimelineEvent *e : pending)
		jPending.push_back(e->to_json());
	j["pending"] = jPending;

	j["active"] = active ? active->to_json() : json(nullptr);

	json jCompleted = json::array();
	for (TimelineEvent *e : completed)
		jCompleted.push_back(e->to_json());
	j["completed"] = jCompleted;

	j["eventTimer"] = eventTimer;

	return j;
}

void Timeline::from_json(const json &j)
{
	for (TimelineEvent *e : pending)
		delete e;
	pending.clear();
	for (TimelineEvent *e : completed)
		delete e;
	completed.clear();
	if (active)
		delete active;
	active = nullptr;

	actors = j.value("actors", std::vector<DialogSequence::DialogActor>{});
	isFinished = j.value("isFinished", false);

	if (j.count("pending"))
		for (const json &je : j.at("pending"))
			pending.push_back(timeline_event_from_json(je));

	if (j.count("active") && !j.at("active").is_null())
		active = timeline_event_from_json(j.at("active"));

	if (j.count("completed"))
		for (const json &je : j.at("completed"))
			completed.push_back(timeline_event_from_json(je));
	
	if (!eventTimer.get())
		eventTimer = t_time_manager::make_independent_timer();
	if (j.count("eventTimer"))
		j.at("eventTimer").get_to(eventTimer);
}

void Timeline::serialize_publish(const SerializeMap &map)
{
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);
	g->add_timer(eventTimer);
}

void Timeline::serialize_initialize(const SerializeMap &map)
{
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);
	context = g;

	// Code made by Claude Sonnet 5 - a portal referenced anywhere in this
	// timeline must start disabled the moment the scenario loads, not only
	// once its owning event's update() first runs. Buildings update before
	// the timeline does each frame, so without this, a portal's own
	// upgrade-tree spawn loop can fire once on the very first frame before
	// the timeline ever gets a chance to say otherwise
	auto disablePortals = [g](TimelineEvent *e)
	{
		if (!e)
			return;
		for (const TimelineSpawnInfo::PortalInstance &portal : e->spawnInfo.spawns)
		{
			BuildingBase *building = g->find_building_by_id(portal.portalId);
			if (!building)
				building = g->find_building_by_pos(portal.spawnOrigin);
			if (building)
				building->spawnEnabled = false;
		}
	};

	disablePortals(active);
	for (TimelineEvent *e : pending)
		disablePortals(e);
	for (TimelineEvent *e : completed)
		disablePortals(e);
}

Timeline::Timeline()
{
	eventTimer = t_time_manager::make_independent_timer();
}

void Timeline::update(t_seconds time)
{
	if (isFinished)
		return;

	// Code made by Claude Sonnet 5 - bootstrap the first event: previously
	// nothing ever promoted pending.front() into active unless active was
	// already non-null, so a freshly built Timeline (active == nullptr,
	// pending populated by add_event) never started at all
	if (!active)
	{
		if (pending.empty())
		{
			isFinished = true;
			return;
		}

		eventTimer->reset(context->get_time());
		active = pending.front();
		pending.pop_front();
	}

	if (active->step(
		context.get(), 
		eventTimer->time_passed(context->get_time())))
	{
		eventTimer->reset(context->get_time());
		completed.push_back(active);
		active = nullptr;
		isFinished = pending.empty();
	}
}

bool Timeline::is_timeline_finished() const
{
	return this->isFinished;
}

std::vector<Timeline::TimelineEvent *> Timeline::get_visible_events(size_t count) const
{
	std::vector<TimelineEvent *> visible;

	if (active && active->showEvent)
		visible.push_back(active);

	for (TimelineEvent *e : pending)
	{
		if (visible.size() >= count)
			break;
		if (e->showEvent)
			visible.push_back(e);
	}

	return visible;
}
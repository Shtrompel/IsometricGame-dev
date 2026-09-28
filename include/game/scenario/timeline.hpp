#pragma once

#include "utils/globals.hpp"
#include "utils/definitions.hpp"
#include "game/game_buildings.hpp"
#include "game/game_entity.hpp"
#include "game/game_grid.hpp"
#include "game/game_bullet.hpp"

/**
 * Mission event order:
 * Intro Dialog -> Initialize Mission -> TutorialBlocking -> Finish Mission -> End Dialog
 * Dialog actor may stay on screen
 */

struct TutorialBlocking
{
	// Disable and grey out all gui
	bool lockGui = false;

	// Pool of buttons to not grey out and disable
	std::vector<std::string> toIgnore;

	// Block camera and map traversal
	bool lockMap = false;

	// Stop all game logic
	bool lockGame = false;

	// Highligh a location/object on the map
	bool doHighlightPos = false;
	IVec highlightPos;

	// Highlight objects on the map
	std::vector<t_id> highlightObjects;
};

void to_json(json &j, const TutorialBlocking &v);
void from_json(const json &j, TutorialBlocking &v);

struct DialogSequence
{

	struct DialogActor
	{
		// Actors can change expressions
		std::vector<t_sprite> spriteHolder;
		// Scale the sprite accordingly
		FVec scale;
		// Range is in [0,1]. On what x ais
		float xPosition;

		// TODO implement dialog audio system
		int synthID = -1;
	};

	/**
	 * All OUT enums move the actor from their current position to below the screen,
	 * IN enums do the opposite.
	 */
	enum class Transition
	{
		IMMEDIATE, //
		// Smoothly move the actor
		SMOOTH_IN,
		SMOOTH_OUT,
		// Actor grows (IN) or goes tiny (OUT) while spinning in place
		SPIN_IN,
		SPIN_OUT,
		// Same as smooth, but with bounce interpolation
		BOUNCE_IN,
		BOUNCH_OUT,
		// Shake the actor in place
		SHAKE,
		// Acts like SHAKE but only once
		SMOOTH,
		// Jump in place
		JUMP
	};

	struct DialogLine
	{
		// Code made by Claude Sonnet 5 - index into Timeline::actors rather
		// than a DialogActor*: actors live in a std::vector shared by every
		// DialogSequence in the timeline, so a raw pointer would dangle the
		// moment that vector reallocates (e.g. a later add_event adds one)
		int actorIndex = -1;
		Transition transition = Transition::IMMEDIATE;
		std::string text;
	};

	std::vector<DialogLine> dialogLines;

	// Code made by Claude Sonnet 5 - progress cursor, same idea as
	// PathData::follower: whoever renders this sequence advances it one
	// line at a time instead of the sequence tracking its own timing
	size_t currentLine = 0;

	DialogSequence()
	{
	}

	void reset()
	{
		currentLine = 0;
	}

	bool is_empty() const
	{
		return dialogLines.empty();
	}

	bool is_finished() const
	{
		return currentLine >= dialogLines.size();
	}
};

void to_json(json &j, const DialogSequence::DialogActor &v);
void from_json(const json &j, DialogSequence::DialogActor &v);

void to_json(json &j, const DialogSequence::DialogLine &v);
void from_json(const json &j, DialogSequence::DialogLine &v);

void to_json(json &j, const DialogSequence &v);
void from_json(const json &j, DialogSequence &v);

struct TimelineSpawnInfo
{
	// Used if there are multiple varyations of spawnable entities
	enum class SpawnType
	{
		IGNORE, // Only the first option is spawned
		RANDOM, // Choose randonly
		LINEAR	// Follow by order
	};

	struct PortalInstance
	{
		// The world id of the object
		t_id portalId{};

		// Is the portal object enabled?
		bool enable = true;

		// Spawn without the use of some portal fallback
		IVec spawnOrigin;

		// How many entities the portal can hold at a single time
		int storedEntityLimit = -1;
		// How many entities can the portal summon in total
		int totalEntityLimit = -1;

		// Intervals between spawning
		t_seconds spawnInterval;

		// ID's of the spawnable objects (values should be able to repeat)
		std::vector<SpawnInfo> spawnPool;

		// Code made by Claude Sonnet 5 - selects how the next spawnPool
		// entry is picked; LINEAR is "spawn in order"
		SpawnType spawnType = SpawnType::LINEAR;

		// Runtime cursor for LINEAR mode - which pool entry fires next.
		// Serialized so a mid-mission save/load doesn't restart the order
		size_t poolIndex = 0;

		// How many entities this portal has spawned in total so far, checked
		// against totalEntityLimit. Serialized for the same reason as
		// poolIndex. storedEntityLimit (how many can be alive at once) isn't
		// enforced yet - that needs tracking which live entities came from
		// this portal, not just a running count
		int totalSpawned = 0;

		// Seconds since this event went RUNNING that this portal is next
		// allowed to spawn - authorable directly in JSON as a stagger
		// offset (e.g. 0, 2, 4 across portals so they don't all start at
		// once), and advances by spawnInterval after each spawn.
		// Serialized for the same reason as poolIndex
		t_seconds nextSpawnAt = 0.f;
	};

	std::vector<PortalInstance> spawns;
};

void to_json(json &j, const TimelineSpawnInfo::PortalInstance &v);
void from_json(const json &j, TimelineSpawnInfo::PortalInstance &v);

void to_json(json &j, const TimelineSpawnInfo &v);
void from_json(const json &j, TimelineSpawnInfo &v);

struct Timeline : Variant
{
public:
	enum class MissionType
	{
		NONE, // Mission is over when dialog is finished
		SURVIVE_TIME,
		SURVIVE_KILLS,
		GAIN_RESOURCES,
		BUILD_BUILDINGS,
		GAIN_POPULATION
	};

	struct TimelineEvent
	{
		// Code made by Claude Sonnet 5 - "type" selects one of the built-in,
		// data-driven mission behaviors (the eventual init/update/is_done
		// would switch on it). The same three methods are also virtual so a
		// one-off scripted mission can subclass and override instead, for
		// something odd enough it isn't worth adding a MissionType for.
		// Ordinary JSON-authored missions should only ever need "type" plus
		// the data fields below; subclassing is the escape hatch, not the
		// default path.
		MissionType type;
		DialogSequence intro, outro;
		TimelineSpawnInfo spawnInfo;
		TutorialBlocking gameLocking;

		// Just the camera to some position as the mission starts
		bool jumpCamera = false;
		IVec jumpTo{};

		// Is there a time limit to this event untill autofail?
		float maxTime = -1.f;

		// Kill all entities that were left before the time ran out
		bool killAll = false;

		// Code made by Claude Sonnet 5 - whether the current/next-event HUD
		// widget may display this event; hidden events are skipped over
		// when picking what to show as current/next
		bool showEvent = true;

		// Code made by Claude Sonnet 5 - the order documented at the top of
		// this file (Intro Dialog -> Initialize Mission -> TutorialBlocking
		// -> Finish Mission -> End Dialog), made explicit as state instead
		// of living only in a comment. TutorialBlocking isn't a separate
		// phase here: gameLocking is meant to stay applied for the whole
		// RUNNING phase (the GUI layer reads it while phase == RUNNING),
		// released once is_done() succeeds and the outro starts.
		enum class Phase
		{
			INTRO_DIALOG,
			RUNNING,
			OUTRO_DIALOG,
			DONE
		};

		Phase phase = Phase::INTRO_DIALOG;

		virtual ~TimelineEvent()
		{
		}

		virtual bool init(GameData *)
		{
			return false;
		}

		// Code made by Claude Sonnet 5 - base implementation pushes
		// spawnInfo.spawns' enable flags onto the matching portals every
		// tick (re-resolved by id/position each time, no stored pointer)
		// and runs each portal's independent spawner. timePassed is
		// seconds since this event went RUNNING (not since the whole
		// program launched), so PortalInstance::nextSpawnAt is authorable
		// as a plain stagger offset in the scenario JSON. Subclasses
		// overriding this should call TimelineEvent::update() first to
		// keep that behavior
		virtual bool update(GameData *, int timePassed);

		virtual bool is_done(GameData *, int)
		{
			return false;
		}

		// Code made by Claude Sonnet 5 - human-readable progress for the
		// current/next-event HUD widget (e.g. "32s left", "3 of 10
		// killed"); empty means the widget just shows the mission name with
		// no extra detail. Only words/digits/spaces - the caller renders
		// this through FastLabel, whose glyph sheet has no punctuation
		virtual std::string get_status_text(GameData *, int) const
		{
			return "";
		}

		// Code made by Claude Sonnet 5 - the non-virtual outer driver: walks
		// intro -> init -> running -> outro -> done, so init/update/is_done
		// never have to re-derive which phase they're in. Advancing intro/
		// outro relies on whatever renders a DialogSequence incrementing its
		// currentLine; this only checks is_finished(). Returns true once the
		// event has fully finished.
		bool step(GameData *data, int timePassed)
		{
			switch (phase)
			{
			case Phase::INTRO_DIALOG:
				if (intro.is_finished())
				{
					init(data);
					phase = Phase::RUNNING;
				}
				break;

			case Phase::RUNNING:
				update(data, timePassed);
				if (is_done(data, timePassed))
					phase = Phase::OUTRO_DIALOG;
				break;

			case Phase::OUTRO_DIALOG:
				if (outro.is_finished())
					phase = Phase::DONE;
				break;

			case Phase::DONE:
				break;
			}

			return phase == Phase::DONE;
		}

		// Base fields only (type, intro, outro, spawnInfo, gameLocking,
		// jumpCamera, jumpTo, maxTime, killAll, phase). Each subclass
		// override calls this first, then adds its own extra fields.
		virtual json to_json() const;
		virtual void from_json(const json &j);

		// Constructs the concrete subclass matching a MissionType (falls
		// back to a plain TimelineEvent for NONE, which has no dedicated
		// subclass - it's the "mission is over when dialog is finished"
		// case, so there's nothing extra to init/update/is_done)
		static TimelineEvent *create(MissionType type);
	};

	struct TimelineEventSurvive : public TimelineEvent
	{
		// How long there is until the mission is won
		int missionTime = -1;

		json to_json() const override;
		void from_json(const json &j) override;

		virtual bool init(GameData *);

		virtual bool is_done(GameData *, int);

		std::string get_status_text(GameData *, int) const override;
	};

	struct TimelineEventKill : public TimelineEvent
	{
		// Hoe many enemy entities the player should kill
		int maxKills = -1;
		// How many specific entities has to be destroyed (can be any)
		std::map<IdPair, int> maxKillPerType;

		// Code made by Claude Sonnet 5 - GameData::deathCounts is a global,
		// monotonic, lifetime counter (never reset), so a mission snapshots
		// it at init() and diffs against that baseline rather than the raw
		// value - otherwise kills from before the mission even started
		// would count towards it. Serialized so save/load mid-mission keeps
		// the right baseline
		long killBaseline = 0;
		std::map<IdPair, long> killTypeBaseline;

		json to_json() const override;
		void from_json(const json &j) override;

		bool init(GameData *) override;
		bool is_done(GameData *, int) override;
		std::string get_status_text(GameData *, int) const override;
	};

	struct TimelineGainResources : public TimelineEvent
	{
		// How much resources should the user gain
		std::map<int, int> resourcesGain;
		// How much resources should be in storage at one time
		std::map<int, int> resourcesStored;

		// Code made by Claude Sonnet 5 - GameData::resourceGained is a
		// global lifetime-gained counter (see GameData::record_resource_gain),
		// so resourcesGain snapshots it at init() and diffs like killBaseline
		// above. resourcesStored has no baseline - it's just a current
		// stock threshold read straight off resourceContext.resources
		std::map<int, long> resourcesBaseline;

		json to_json() const override;
		void from_json(const json &j) override;

		bool init(GameData *) override;
		bool is_done(GameData *, int) override;
		std::string get_status_text(GameData *, int) const override;
	};

	struct TimelineGainBuild : public TimelineEvent
	{
		// How much of every building the player should
		std::map<t_id, int> buildingCounter;

		json to_json() const override;
		void from_json(const json &j) override;

		bool is_done(GameData *, int) override;
		std::string get_status_text(GameData *, int) const override;
	};

	struct TimelineGainPopulation : public TimelineEvent
	{
		// How much entities of every type should spawn
		std::map<IdPair, int> entitiyTypeSpawn;
		// How much entities of every type should be in total
		std::map<IdPair, int> entitiyTypeTotal;
		// How much entities who are citisens should spawn
		int entitiesSpawn = -1;
		// How much entities there should be in total
		int entitiesPopulation = -1;

		// Code made by Claude Sonnet 5 - same baseline-diff reasoning as
		// TimelineEventKill::killBaseline: entitiesSpawn/entitiyTypeSpawn
		// are deltas since mission start, read off GameData::creationCounts
		long citizenBaseline = 0;
		std::map<IdPair, long> typeBaseline;

		json to_json() const override;
		void from_json(const json &j) override;

		bool init(GameData *) override;
		bool is_done(GameData *, int) override;
		std::string get_status_text(GameData *, int) const override;
	};

	Timeline();

	~Timeline()
	{
		for (TimelineEvent *e : completed)
			delete e;
		for (TimelineEvent *e : pending)
			delete e;
		if (active)
			delete active;
	}

	template <class T>
	void add_event(T &&t)
	{
		// Code made by Claude Sonnet 5 - T&& is a forwarding reference, so T
		// deduces to a reference type (e.g. "Foo&") when called with an
		// lvalue; decay it before using it as the storage/new type
		using Stored = std::decay_t<T>;
		Stored *ptr = new Stored(std::forward<T>(t));
		add_event<Stored>(ptr);
	}

	template <class T>
	void add_event(T *t)
	{
		static_assert(
			std::is_base_of<TimelineEvent, T>());
		pending.push_back(
			dynamic_cast<TimelineEvent *>(t));
	}

	void update(t_seconds time);

	bool is_timeline_finished() const;

	// Code made by Claude Sonnet 5 - walks active then pending in order,
	// skipping events with showEvent == false, and returns up to `count`
	// of the ones that are visible (e.g. count=2 for a current/next HUD)
	std::vector<TimelineEvent *> get_visible_events(size_t count) const;

	json to_json() const override;
	void from_json(const json &j) override;
	void serialize_publish(const SerializeMap &map) override;
	void serialize_initialize(const SerializeMap &map) override;

public:
	VariantPtr<GameData> context{};
	std::vector<DialogSequence::DialogActor> actors;

	bool isFinished = false;

	std::deque<TimelineEvent *> pending{};
	TimelineEvent *active = nullptr;
	std::deque<TimelineEvent *> completed{};

	t_body_timer eventTimer;
};

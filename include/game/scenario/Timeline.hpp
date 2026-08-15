#pragma once

#include "../../utils/globals.hpp"
#include "game/game_buildings.hpp"
#include "game/game_entity.hpp"
#include "game/game_grid.hpp"
#include "game/game_bullet.hpp"

struct GameData;



struct Timeline : Variant
{
public:
	enum class MissionType
	{
		SURVIVE_TIME,
		SURVIVE_KILLS,
		GAIN_RESOURCES,
		BUILD_BUILDINGS,
		GAIN_POPULATION
	};

	struct TimelineMission
	{
		MissionType type;

		virtual bool init(GameData*);

		virtual bool update(GameData*);

		virtual bool is_done(GameData*);
	};

	struct TimelineEvent
	{

		TimelineEvent()
		{

		}
		
		virtual ~TimelineEvent() {}

		bool is_event_finished(GameData*);

		void update(GameData*);

		std::vector<TimelineMission*> missions;
	};

	Timeline()
	{
	}

	~Timeline()
	{
		for (TimelineEvent* e : completed)
			delete e;
		for (TimelineEvent* e : pending)
			delete e;
		if (active)
			delete active;
	}

	template <class T>
	void add_event(T&& t)
	{
		T* ptr = new T(std::forward(t));
		add_event<T>(ptr);
	}

	template <class T>
	void add_event(T* t)
	{
		static_assert(
			std::is_base_of<TimelineEvent, T>());
		pending.push_back(
			dynamic_cast<TimelineEvent*>(t));
	}

	void update(t_seconds time);

	bool is_timeline_finished() const;

public:

	VariantPtr<GameData> context{};

	bool isFinished = false;

	std::deque<TimelineEvent*> pending{};
	TimelineEvent* active = nullptr;
	std::deque<TimelineEvent*> completed{};

};


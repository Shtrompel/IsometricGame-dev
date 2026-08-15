#include "game/scenario/Timeline.hpp"

#include "game/game_data.hpp"

void Timeline::update(t_seconds time)
{
	if (!active && pending.empty() && completed.empty())
		return;

	if (isFinished)
		return;

	if (!active)
	{
		WARNING("active is null! This shouldn't happen!");
	}
	else
	{
		
		active->update(context.get());
		if (active->is_event_finished(context.get()))
		{
			completed.push_back(active);

			if (pending.size())
			{
				this->active = pending.front();
				pending.pop_front();
			}
			else
			{
				isFinished = true;
			}
		}

	}
}

bool Timeline::is_timeline_finished() const
{
	return this->isFinished;
}

bool Timeline::TimelineEvent::is_event_finished(GameData* data)
{
	for (TimelineMission* mission : missions)
	{
		if (!mission->is_done(data))
		{
			return false;
		}
	}
	return true;
}

void Timeline::TimelineEvent::update(GameData* data)
{
	for (TimelineMission* mission : missions)
	{
		mission->update(data);
	}
}

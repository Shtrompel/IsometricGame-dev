#ifndef _GAME_PATHFIND
#define _GAME_PATHFIND

#define _WIN32_WINNT 0x0501

#include "game/game_data.hpp"

#include <future>
#include <thread>
#include <queue>


#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#ifndef BOOST_ASIO_NO_EXCEPTIONS
#undef BOOST_ASIO_NO_EXCEPTIONS
#endif

#include <boost/thread.hpp>


#ifdef __GNUC__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#endif // __GNUC__

typedef
struct DEFAULT_PATHFIND_CONDITION
{
	constexpr bool operator()(const sf::Vector2i&, const GameBody*, const int) const
	{
		return true;
	}
}
t_pathfind_condition;

struct PathfindNode
{
	bool hasPrev = false;
	bool properInit = false;
	sf::Vector2i tilePos;
	unsigned gScore = 0u;
	unsigned hScore = 0u;
	sf::Vector2i prev;
	bool inOpenSet = false, inClosedSet = false;

	PathfindNode() {}

	PathfindNode(sf::Vector2i tilePos, unsigned hScore)
		: tilePos(tilePos), hScore(hScore)
	{
		properInit = true;
	}

	unsigned f(const int dijkstra, const int greed) const
	{
		return dijkstra * gScore + greed * hScore;
	}


};

//https://github.com/daancode/a-star/blob/master/source/AStar.cpp
inline PathData generate_path(
	const sf::Vector2i &start,
	const sf::Vector2i &end,
	const Chunks *chunks,
	const int dijkstra,
	const int greed,
	float radius = -1.f,
	bool ignoreBarriers = false)
{
	const auto isBarrier = [chunks, ignoreBarriers](const sf::Vector2i &pos) {
		if (ignoreBarriers)
		  return false;
		const Tile *tile = chunks->get_tile_safe(pos.x, pos.y);
		return (tile && tile->is_barrier());
	};

	const auto euclideanHeuristicDist = [](const sf::Vector2i &a, const sf::Vector2i &b) {
		return math_sqrt<int>(100 * ((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)));
	};

	std::unordered_map<Vec<int>, PathfindNode> hashGrid;

	const auto openSetCmp = [dijkstra, greed](const PathfindNode *a, const PathfindNode *b) 
	{
		unsigned fa, fb;
		fa = a->f(dijkstra, greed);
		fb = b->f(dijkstra, greed);
		if (fa != fb) 
			return fa > fb;
		return a->hScore > b->hScore;
	};
	std::priority_queue<PathfindNode *, std::vector<PathfindNode *>, decltype(openSetCmp)> openSet(openSetCmp);

	if (isBarrier(start))
	{
		WARNING("Can't generate path that starts in a barrier");
		return {};
	}
	else
	{
		hashGrid[start] =
			PathfindNode(start, euclideanHeuristicDist(start, end)); //int_abs(start.x - end.x) + int_abs(start.y - end.y);
		openSet.push(&hashGrid[start]);
	}
	unsigned iterationCount = 0;
	PathfindNode *best = nullptr;
	
	if (start == end)
	  return {};

	while (!openSet.empty() && iterationCount < BIG_INFINITY)
	{
		++iterationCount;

		// Find node with smalleat f value
		best = openSet.top();
		openSet.pop();

		if (best->inClosedSet)
			continue;

		best->inClosedSet = true;

		// Iterate through all 8 directions
		for (int i = 0; i < 8; i++)
		{
			const sf::Vector2i &dir = DIRECTIONS[i];
			sf::Vector2i pos = best->tilePos + dir;
			bool isDiag = (math_abs(dir.x) + math_abs(dir.y)) == 2;

			// Skip unaccesible nodes
			bool skip = false;
			if (isDiag)
			{
				skip |= isBarrier(best->tilePos + sf::Vector2i{dir.x, 0}) ||
						isBarrier(best->tilePos + sf::Vector2i{0, dir.y});
			}
			auto itClosed = hashGrid.find(pos);
			skip = skip || (itClosed != hashGrid.end() && itClosed->second.inClosedSet);
			skip = skip || isBarrier(pos);
			
			if (skip && pos != end)
			{
				continue;
			}

			// Euclidean heuristic
			unsigned newG = best->gScore + (isDiag ? 14 : 10);

			auto itNode = hashGrid.find(pos);
			bool isNew = (itNode == hashGrid.end());
			if (isNew || newG < itNode->second.gScore)
			{
				PathfindNode &successor = hashGrid[pos];
				successor.prev = best->tilePos;
				successor.tilePos = pos;
				successor.hasPrev = true;
				successor.gScore = newG;
				successor.hScore = euclideanHeuristicDist(pos, end);
				successor.inOpenSet = true;
				openSet.push(&successor);
			}
		}

		// If the final node was reached stop the algorithm
		if (best->tilePos == end)
			break;
	}

	if (!best)
		return {};
	PathfindNode &tile = *best;

	iterationCount = 0;
	sf::Vector2i lastDelta = {0, 0};
	PathData ret;

	// Remove any useless neightboring nodes that are
	// in the same direction.
	bool keepGoing = true;
	int index = (int)BIG_INFINITY - 1; // Doesn't have to start at zero
	do
	{
		++iterationCount;
		sf::Vector2i delta;
		delta = tile.tilePos;
		if (tile.hasPrev)
			delta -= hashGrid.at(tile.prev).tilePos;
		if (delta != lastDelta || !tile.hasPrev)
			ret.path.push_front({index--, tile.tilePos});

		if (hashGrid.count(tile.prev))
			tile = hashGrid.at(tile.prev);
		else
			keepGoing = false;

		lastDelta = delta;
		if (tile.tilePos == start)
		{
			ret.path.push_front({index, tile.tilePos});
			break;
		}
		if (iterationCount > BIG_INFINITY)
		{
			assert(0);
			break;
		}
	} while (keepGoing);

	if (chunks->has_tile(end.x, end.y))
	{
		ret.destination = &chunks->get_tile(end.x, end.y);
		ret.destPos = end;
	}

	//DEBUG("Path Start: %d %d", ret.back().x, ret.back().y);
	//DEBUG("Path End: %d %d\n", ret.front().x, ret.front().y);

	//DEBUG("%d : %s", out.valid(), out.to_string().c_str());
	//DEBUG("%s", out.why_invalid().c_str());

	return ret;
}

inline void pathdata_update_path(
	PathData& pathData,
	const IVec& end,
	const Chunks* chunks,
	const int dijkstra,
	const int greed,
	const IVec& start,
	float radius = -1.f,
	bool ignoreBarriers = false
)
{
	/*
	IVec start;

	if (pathData.path.size() > 1)
	{
		//pathData.path.pop_back();
	}
	start = pathData.path.front().value;
	*/
	PathData pathAdditional = generate_path(
		start,
		end,
		chunks,
		dijkstra,
		greed,
		radius,
		ignoreBarriers
	);

	pathData.append(pathAdditional);
	pathData.destPos = pathAdditional.destPos;
	pathData.destination = pathAdditional.destination;
}

template <
	typename EnumType = BodyType,
	std::enable_if_t<
	std::is_enum<EnumType>::value,
	bool> = true,
	class Condition = t_pathfind_condition>
static PathData find_build_path(
	GameData* context,
	Chunks* chunks,
	const FVec& originPath = { 0.0f, 0.0f },
	const FVec& originSearch = { 0.0f, 0.0f },
	const t_idpair treeId = t_idpair{ ENUM_BODY_TYPE, (t_id)BodyType::BUILDING },
	float maxRadius = -1.f,
	Condition condition = DEFAULT_PATHFIND_CONDITION())
{
	const int dijkstra = (int)context->get_const(
		t_constnum::A_STAR_DIJKSTRA_VALUE);
	const int greed = (int)context->get_const(
		t_constnum::A_STAR_GREED_VALUE);
	std::chrono::time_point<std::chrono::system_clock>
		chronoStart, chronoEnd;
	chronoStart = std::chrono::system_clock::now();

	std::list<BuildingBody*> list;

	// Todo
	// Find better performing way of doing this
	// Check if path was changes in the middle of operation

	PathData bestPath;
	int bestDist = INT_MAX;

	// Lambda that finds that path for every close building,
	// if it returns false the quadtree will keep searching more.
	auto comparePaths = [&condition, &bestDist,
		&bestPath, chunks, &originPath,
		&dijkstra, &greed, &maxRadius](
			const sf::Vector2i& pos,
			const GameBody* body,
			const int distsq) -> bool
		{
			assert(body);
			if (body->type != BodyType::BUILDING)
				LOG_ERROR("Body type is %d", (int)body->type);
			ASSERT_ERROR(body->type == BodyType::BUILDING, "Can't search for non building bodies.");

			if (maxRadius != 1.f &&
				distsq > maxRadius * maxRadius)
				false;

			const BuildingBody* build = dynamic_cast<const BuildingBody*>(body);
			assert(body);
			assert(build);
			bool pass = condition(build->tilePos, build, distsq);
			// If the condition is passed
			if (pass)
			{
				// Try to find the path
				auto pathData = generate_path(
					vec_pos_to_tile(originPath),
					build->tilePos,
					chunks,
					dijkstra,
					greed);

				// If path isn't empty and ends in the destination
				if (pathData.valid() && (distsq < bestDist))
				{
					bestDist = distsq;
					bestPath = std::move(pathData);
					//LOG("Candidate: %d %s %s", distsq, vec_str(build->tilePos).c_str(), build->name);
				}
				else
				{
					pass = false;
				}
			}
			return pass;
		};

	// Get the nearest building

	list = context->nearest_buildings_quad(
		originSearch,
		1,
		treeId,
		comparePaths);

	if (list.size())
		bestPath = generate_path(
			vec_pos_to_tile(originPath),
			list.back()->tilePos,
			context->chunks,
			dijkstra,
			greed);

	if (list.empty())
		return PathData{};

	if (!bestPath.valid())
	{
		LOG("Can't invalid path: %s", bestPath.why_invalid().c_str());
		assert(0);
	}

	// For debugging
	chronoEnd = std::chrono::system_clock::now();
	std::chrono::duration<t_seconds> elapsedSeconds =
		chronoEnd - chronoStart;
	t_seconds timeDif = elapsedSeconds.count();

	if (timeDif > MIN_TIME_PATHFIND)
		WARNING(
			"Pathfind algorithms takes too long, %f seconds where %f is the max.",
			timeDif,
			MIN_TIME_PATHFIND);

	return bestPath;
}

template <
	typename EnumType = BodyType,
	std::enable_if_t<
	std::is_enum<EnumType>::value,
	bool> = true,
	class Condition = t_pathfind_condition>
static PathData find_building_and_path(
	GameData *context,
	Chunks *chunks,
	const sf::Vector2f &entityPos = {0.0f, 0.0f},
	const t_idpair treeId = t_idpair{ENUM_BODY_TYPE, (t_id)BodyType::BUILDING},
	float maxRadius = -1.f,
	Condition condition = DEFAULT_PATHFIND_CONDITION())
{
	return find_build_path
	(
		context,
		chunks,
		entityPos,
		entityPos,
		treeId,
		maxRadius,
		condition
	);
}

template <
	typename EnumType = BodyType,
	std::enable_if_t<
	std::is_enum<EnumType>::value,
	bool> = true,
	class Condition = t_pathfind_condition>
static PathData find_building_and_path(
	GameData* context,
	const sf::Vector2f& entityPos,
	const t_idpair treeId = t_idpair{ ENUM_BODY_TYPE, (t_id)BodyType::BUILDING },
	float maxRadius = -1.f,
	Condition condition = GameData::DEFAULT_CONDITION())
{
	return find_building_and_path(
		context,
		context->chunks,
		entityPos,
		treeId,
		maxRadius,
		condition);
}

typedef std::function<void(void)> t_def_findbuilding;


struct PathfindRequest
{
	GameData* context = nullptr;
	FVec pos = {};
	t_idpair id = {};
	float maxRadius = -1.f;
	std::function< bool(const sf::Vector2i&, const GameBody*, const int)>
		condition{};

	std::promise<PathData> promise{};

	~PathfindRequest()
	{
	}
};

template <class Out>
struct QueryThreadInstance
{
	static constexpr auto TIME_ZERO = std::chrono::seconds(0);
	static constexpr auto READY = std::future_status::ready;
	std::shared_future<Out> m_future;

	QueryThreadInstance( QueryThreadInstance<Out>&& other) noexcept
	{
		m_future = std::move(other.m_future);
	}

	QueryThreadInstance(std::shared_future<Out>&& future)
	{
		m_future = std::move(future);
	}

	QueryThreadInstance()
	{
	}

	bool ready() const
	{
		return m_future.wait_for(TIME_ZERO) == READY;
	}

	Out get()
	{
		//m_future.wait();
		assert(m_future.wait_for(TIME_ZERO) == READY);
		return m_future.get();
	}
};

typedef QueryThreadInstance<PathData> t_path_instance;

struct QueryPoolPath
{

	// Threads
	size_t threadCount;
	std::unique_ptr<boost::asio::thread_pool> workers;
	// Alows shared time locks between threads

	boost::interprocess::interprocess_semaphore queue;
	boost::asio::thread_pool pool;

	typedef std::shared_ptr < PathfindRequest > t_ptr_request;

	std::atomic_size_t threadCounter = 0;
	// size_t theadCount = 0;
	std::mutex mutexRequests;
	std::queue< std::shared_ptr<PathfindRequest>> requestQueue;
	GameData* context;
	bool bForceReset = false;


	QueryPoolPath(
		GameData* context,
		std::size_t threadCount,
		unsigned tasksLimit);

	t_ptr_request generate_request() const;

	std::shared_future<PathData> add_request(t_ptr_request request);

	void add_thread();

	void wait() const;

	void force_reset();

	void reset();

};

#define FORCE_SINGLE_THREAD

template <
	typename EnumType = BodyType,
	std::enable_if_t<
	std::is_enum<EnumType>::value,
	bool> = true,
	class Condition = t_pathfind_condition>
static QueryThreadInstance<PathData> add_request_find_building(
	QueryPoolPath& pool,
	GameData* context,
	const sf::Vector2f& entityPos,
	const t_idpair treeId = t_idpair{ ENUM_BODY_TYPE, (t_id)BodyType::BUILDING },
	Condition condition = DEFAULT_PATHFIND_CONDITION())
{
	typedef PathData Out;

#ifndef FORCE_SINGLE_THREAD
	// Create processing lambda
	/*
	const auto func = [context, entityPos, treeId, condition]() -> PathData {
		Out&& out = find_building_and_path(
			context,
			entityPos,
			treeId,
			-1.f, // todo change to radius_max from entity
			condition);
		return std::move(out);
		};*/

	auto request = pool.generate_request();
	request->context = context;
	request->id = treeId;
	request->condition = condition;
	request->maxRadius = -1.f;
	request->pos = entityPos;

	std::shared_future<PathData> future = pool.add_request(request);
	assert(future.valid());

	return QueryThreadInstance<PathData>(std::move(future));
#else
	Out out = find_building_and_path(context, context->chunks, entityPos, treeId, -1.f, condition);
	std::promise<Out> promise;
	promise.set_value(out);
	return QueryThreadInstance<PathData>(std::move(promise.get_future()));
#endif
}

/*
template <
	typename EnumType = BodyType,
	std::enable_if_t<
	std::is_enum<EnumType>::value,
	bool> = true,
	class Condition = t_pathfind_condition>
static QueryThreadInstance<PathData> add_request_find_building(
	QueryThreadPool& pool,
	GameData* context,
	const sf::Vector2f& entityPos,
	const t_idpair treeId = t_idpair{ ENUM_BODY_TYPE, (t_id)BodyType::BUILDING },
	Condition condition = DEFAULT_PATHFIND_CONDITION())
{
	typedef PathData Out;

#ifndef FORCE_SINGLE_THREAD
	// Create processing lambda
	const auto func = [context, entityPos, treeId, condition]() -> PathData {
		Out&& out = find_building_and_path(
			context,
			entityPos,
			treeId,
			-1.f, // todo change to radius_max from entity
			condition);
		return std::move(out);
		};
	
	

	return QueryThreadInstance(pool.add(func));
#else
	Out out = find_building_and_path(context, context->chunks, entityPos, treeId, -1.f, condition);
	std::promise<Out> promise;
	promise.set_value(out);
	return QueryThreadInstance(promise.get_future());
#endif
}
*/

#ifdef __GNUC__

#pragma GCC diagnostic pop

#endif // __GNUC__

#endif // _GAME_PATHFIND
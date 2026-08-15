#include "utils/pathfind.hpp"

// Definitions of non‑template member functions of QueryPoolPath

QueryPoolPath::QueryPoolPath(
	GameData* context,
	std::size_t threadCount,
	unsigned tasksLimit) 
	: threadCount(threadCount), queue(tasksLimit)
{
	reset();
}

QueryPoolPath::t_ptr_request QueryPoolPath::generate_request() const
{
	return std::make_shared<PathfindRequest>();
}

std::shared_future<PathData> QueryPoolPath::add_request(t_ptr_request request)
{
	request->promise = std::promise<PathData>();

	mutexRequests.lock();
	requestQueue.push(request);
	mutexRequests.unlock();

	if (threadCounter.load() == 0)
		add_thread();

	return request->promise.get_future().share();
}

void QueryPoolPath::add_thread()
{
	threadCounter.store(threadCounter.load() + 1);

	auto func = std::bind([](
		std::atomic_size_t* const threadCounter,
		bool* const bForceReset,
		GameData* const context,
		std::queue< t_ptr_request>* requestQueue,
		std::mutex* const mutexRequests
		) {
			while (!(*bForceReset))
			{
				mutexRequests->lock();
				if (requestQueue->empty())
				{
					mutexRequests->unlock();
					break;
				}
				t_ptr_request& back = requestQueue->front();
				requestQueue->pop();
				mutexRequests->unlock();

				PathData&& out = find_building_and_path(
					back->context,
					back->pos,
					back->id,
					back->maxRadius,
					back->condition);

				try
				{
					back->promise.set_value(PathData{ out });
				}
				catch (...)
				{
					std::exception_ptr e = std::current_exception();
					LOG_ERROR("Error!");
					try
					{
						std::rethrow_exception(e);
					}
					catch (const std::exception& e)
					{
						LOG_ERROR("The error: %s", e.what());
					}
				}
			}

			threadCounter->store(threadCounter->load() - 1);

		}, &threadCounter, &bForceReset, context, &requestQueue, &mutexRequests);

	std::thread t1(func);
	t1.join();
	/*
	boost::asio::post(
		pool,
		func);// , requestQueue, & mutexRequests));*/
}

void QueryPoolPath::wait() const
{
	workers->wait();
}

void QueryPoolPath::force_reset()
{
	bForceReset = true;
	requestQueue = std::queue< t_ptr_request>();
	reset();
}

void QueryPoolPath::reset()
{
	// Wait untill all threads are stopped, then clear them
	if (workers)
	{
		wait();
		workers.reset();
	}
	// Make a new thread_pool instance
	workers = std::make_unique<boost::asio::thread_pool>(threadCount);
}
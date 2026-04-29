#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <ranges>
#include <thread>

namespace HomeCompa::Util
{

class ThreadPool
{
public:
	struct Initializer
	{
		unsigned int threadCount { std::thread::hardware_concurrency() };
		size_t       maxQueueSize { std::numeric_limits<size_t>::max() };
	};

public:
	explicit ThreadPool(const Initializer& initializer = {})
		: m_maxQueueSize { initializer.maxQueueSize }
	{
		m_threads.reserve(initializer.threadCount);
		std::ranges::transform(std::views::iota(decltype(initializer.threadCount) { 0 }, initializer.threadCount), std::back_inserter(m_threads), [this](auto) {
			return std::jthread(std::bind_front(&ThreadPool::work, this));
		});
	}

	void enqueue(std::function<void()> task)
	{
		{
			std::unique_lock lock(m_tasksGuard);
			m_condition.wait(lock, [this] {
				return m_tasks.size() < m_maxQueueSize;
			});
			m_tasks.emplace(std::move(task));
		}

		m_condition.notify_one();
	}

	void wait()
	{
		{
			std::unique_lock lock(m_tasksGuard);
			m_condition.wait(lock, [this] {
				return m_tasks.empty();
			});
		}

		m_threads.clear();
	}

private:
	std::function<void()> getTask(const std::stop_token& stop)
	{
		std::unique_lock lock(m_tasksGuard);
		if (!m_condition.wait(lock, stop, [this] {
				return !m_tasks.empty();
			}))
			return {};

		auto task = std::move(m_tasks.front());
		m_tasks.pop();
		m_condition.notify_all();

		return task;
	}

	void work(const std::stop_token stop)
	{
		while (!stop.stop_requested())
			if (const auto task = getTask(stop))
				task();
	}

private:
	const size_t                      m_maxQueueSize;
	std::queue<std::function<void()>> m_tasks;
	std::mutex                        m_tasksGuard;
	std::condition_variable_any       m_condition;
	std::vector<std::jthread>         m_threads;
};

} // namespace HomeCompa::Util

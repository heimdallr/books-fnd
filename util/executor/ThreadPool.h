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
	explicit ThreadPool(const int numThreads = static_cast<int>(std::thread::hardware_concurrency()))
	{
		m_threads.reserve(static_cast<size_t>(numThreads));
		std::ranges::transform(std::views::iota(0, numThreads), std::back_inserter(m_threads), [this](auto) {
			return std::jthread(std::bind_front(&ThreadPool::work, this));
		});
	}

	void enqueue(std::function<void()> task)
	{
		{
			std::unique_lock lock(m_tasksGuard);
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
		m_condition.notify_one();

		return task;
	}

	void work(const std::stop_token stop)
	{
		while (!stop.stop_requested())
			if (const auto task = getTask(stop))
				task();
	}

private:
	std::queue<std::function<void()>> m_tasks;
	std::mutex                        m_tasksGuard;
	std::condition_variable_any       m_condition;
	std::vector<std::jthread>         m_threads;
};

} // namespace HomeCompa::Util

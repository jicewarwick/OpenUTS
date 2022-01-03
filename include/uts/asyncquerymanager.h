#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>

enum class QueryCondition {
	Failed,
	Initialized,
	OnGoing,
	Timeout,
	Succcess,
};

class ASyncQueryManager {
public:
	ASyncQueryManager();
	ASyncQueryManager(std::function<void()> func, std::chrono::milliseconds timeout);
	~ASyncQueryManager();

	void set_func(std::function<void()> func) { func_ = func; }
	void set_timeout(std::chrono::milliseconds timeout) { timeout_ = timeout; }

	QueryCondition condition() const { return condition_; };
	[[nodiscard]] QueryCondition query(uint num_tries = 1);
	void done(bool success);

private:
	std::function<void()> func_;
	std::chrono::milliseconds timeout_;

	QueryCondition condition_{QueryCondition::Initialized};

	std::mutex mutex_;
	std::condition_variable cv_;
};

#include "asyncquerymanager.h"

#include <ranges>
#include <thread>

using std::function, std::chrono::milliseconds, std::unique_lock;

ASyncQueryManager::ASyncQueryManager() {}

ASyncQueryManager::ASyncQueryManager(function<void()> func, milliseconds timeout, milliseconds wait_time)
	: func_(func), timeout_(timeout), wait_time_(wait_time) {}

ASyncQueryManager::~ASyncQueryManager() {}

QueryCondition ASyncQueryManager::query(uint num_tries) {
	for (uint _ : std::views::iota(num_tries)) {
		std::cv_status cv_status;
		{
			unique_lock lock(mutex_);
			condition_ = QueryCondition::OnGoing;
			func_();
			cv_status = cv_.wait_for(lock, timeout_);
		}
		if (cv_status == std::cv_status::timeout) { condition_ = QueryCondition::Timeout; }
		if (condition_ == QueryCondition::Succcess) { return condition_; }
		std::this_thread::sleep_for(wait_time_);
	}
	return condition_;
}

void ASyncQueryManager::done(bool success) {
	condition_ = success ? QueryCondition::Succcess : QueryCondition::Failed;
	cv_.notify_one();
}

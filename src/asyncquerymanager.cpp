#include "asyncquerymanager.h"

#include <ranges>

using std::function, std::chrono::milliseconds, std::unique_lock;

ASyncQueryManager::ASyncQueryManager() {}

ASyncQueryManager::ASyncQueryManager(function<void()> func, milliseconds timeout) : func_(func), timeout_(timeout) {}

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
	}
	return condition_;
}

void ASyncQueryManager::done(bool success) {
	condition_ = success ? QueryCondition::Succcess : QueryCondition::Failed;
	cv_.notify_one();
}

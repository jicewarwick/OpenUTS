#pragma once

#include <chrono>
#include <list>
#include <thread>

/**
 * @brief 限速器
 * @tparam Duration 时间单位
 *
 * @details 建立单例后调用`wait()`. 若`dur`时间内, `wait()`调用的次数大于`rate`则堵塞线程.
 */
template <class Duration>
class RateThrottler {
public:
	/**
	 * @brief
	 * @param rate 单位时间内的可调用次数
	 * @param dur 单位时间
	 */
	RateThrottler(int rate, Duration dur) : rate_(rate), dur_(dur) {}

	/**
	 * @brief 使用限速器. 若调用次数没有超限则立即返回. 若次数超过限制, 则等待至时间单位结束后返回.
	 */
	void wait() {
		if (rate_ > 0) {
			auto now = std::chrono::steady_clock::now();
			if (request_time_list_.size() < rate_) {
				request_time_list_.push_back(now);
			} else {
				CleanList();
				if (request_time_list_.size() >= rate_) {
					auto end_time = request_time_list_.front() + dur_;
					std::this_thread::sleep_until(end_time);
				}
				request_time_list_.push_back(std::chrono::steady_clock::now());
			}
		}
	}

private:
	int rate_;
	Duration dur_;
	std::list<std::chrono::steady_clock::time_point> request_time_list_{};
	void CleanList() {
		auto now = std::chrono::steady_clock::now();
		auto threshold = now - dur_;
		while ((!request_time_list_.empty()) && (request_time_list_.front() < threshold)) {
			request_time_list_.pop_front();
		}
	}
};

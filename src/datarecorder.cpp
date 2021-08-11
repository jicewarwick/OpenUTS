#include "datarecorder.h"

#include <spdlog/spdlog.h>

DataRecorder::DataRecorder() { worker_ = std::thread(&DataRecorder::Process, this); }

DataRecorder::~DataRecorder() {
	spdlog::trace("destroying data recorder");
	working_ = false;
	cv_.notify_one();
	worker_.join();
}

void DataRecorder::DataSink(const MarketDepth& data) {
	{
		std::scoped_lock _(mutex_);
		q_.push(data);
	}
	cv_.notify_one();
}

void DataRecorder::Process() {
	MarketDepth data;
	while (working_) {
		spdlog::trace("still working.");
		{
			spdlog::trace("waiting.");
			std::unique_lock lock_(mutex_);
			if (q_.empty()) { cv_.wait(lock_); }
			spdlog::trace("woke up.");
		}
		while (!q_.empty()) {
			{
				std::scoped_lock _(mutex_);
				data = q_.front();
				q_.pop();
			}
			WriteDB(data);
		}
	}
	spdlog::trace("no longer working");
}

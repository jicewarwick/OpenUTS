#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include <uts/data_struct.h>

/**
 * @brief 行情数据记录气基类，通过不断调用其 `DataSink` 函数记录行情数据。子类需实现 `WriteDB` 函数完成数据如入库
 * @details 该类内部有一个数据写入队列，可应对段时间的大量数据写入请求
 */
class DataRecorder {
public:
	DataRecorder();
	DataRecorder(const DataRecorder&) = delete;
	DataRecorder& operator=(const DataRecorder&) = delete;
	virtual ~DataRecorder();

	/// 接收的行情数据
	void DataSink(const MarketDepth& data);

protected:
	/**
	 * @brief 数据记录实现
	 * @param data 新收到的行情数据
	 */
	virtual void WriteDB(const MarketDepth& data) = 0;

private:
	std::thread worker_;
	std::mutex mutex_;
	std::queue<MarketDepth> q_;
	std::condition_variable cv_;
	bool working_ = true;

	void Process();
};

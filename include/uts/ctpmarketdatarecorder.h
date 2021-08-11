#pragma once

#include <uts/ctpmarketdata.h>
#include <uts/datarecorder.h>

/**
 * @brief 基于 `CTP` 的行情记录器
 */
class CTPMarketDataRecorder : public CTPMarketData {
public:
	/**
	 * @brief 构造函数. 完成构造需设定 `DataRecorder` 后可通过 `Login` 开始记录行情数据
	 * @param server_addr `CTP` 行情服务器地址，为可同时提供多个服务器地址同 `CTP` 底层自动切换
	 */
	CTPMarketDataRecorder(const std::vector<IPAddress>& server_addr) : CTPMarketData(server_addr) {}
	CTPMarketDataRecorder(const CTPMarketDataRecorder&) = delete;
	CTPMarketDataRecorder& operator=(const CTPMarketDataRecorder&) = delete;
	~CTPMarketDataRecorder() { data_recorder_ = nullptr; }

	/**
	 * @brief 指定数据记录器。系统提供 `CSVDataRecorder` 和 `SQLite3DataRecorder`
	 * @param data_recorder 指向 `DataRecorder` 的指针
	 */
	void setSink(DataRecorder* data_recorder) { data_recorder_ = data_recorder; }

private:
	DataRecorder* data_recorder_ = nullptr;

	void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) override;
};

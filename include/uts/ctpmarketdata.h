#pragma once

#include <condition_variable>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <CTP/ThostFtdcMdApi.h>
#include <CTP/ThostFtdcUserApiDataType.h>
#include <uts/asyncquerymanager.h>
#include <uts/data_struct.h>
#include <uts/market_data.h>
#include <uts/observer.h>

/**
 * @brief CTP行情基类
 * @note 已完成登录登出和订阅退订功能. 继承后可定制行情处理
 */
class CTPMarketDataBase : public MarketDataSource, private CThostFtdcMdSpi {
public:
	CTPMarketDataBase(const std::vector<IPAddress>& server_addr);
	CTPMarketDataBase(const IPAddress& server_addr) : CTPMarketDataBase(std::vector<IPAddress>{server_addr}) {}
	CTPMarketDataBase(const CTPMarketDataBase&) = delete;
	CTPMarketDataBase& operator=(const CTPMarketDataBase&) = delete;
	~CTPMarketDataBase() override;

	void LogIn() override;
	void LogOut() noexcept override;

	void Subscribe(const std::vector<Ticker>& ticker_list) override;
	void Unsubscribe(const std::vector<Ticker>& ticker_list) override;

protected:
	void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) override = 0;

private:
	CThostFtdcMdApi* md_api_ = nullptr;

	int request_id_ = 0;
	std::filesystem::path cache_path_;

	ASyncQueryManager log_in_query_manager_{std::bind(&CTPMarketDataBase::LogInASync, this)};
	ASyncQueryManager log_out_query_manager_{std::bind(&CTPMarketDataBase::LogOutASync, this)};
	ASyncQueryManager flexible_query_manager_;

	// login and logout
	void LogInASync() noexcept;
	void LogOutASync() noexcept;
	void OnFrontConnected() override;
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID,
						bool bIsLast) override;
	void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID,
						 bool bIsLast) override;

	// subscription
	void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo,
							int nRequestID, bool bIsLast) override;
	void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo,
							  int nRequestID, bool bIsLast) override;
};

/**
 * @brief CTP行情源
 *
 * CTP行情源. 构造时需提供经纪商行情服务器地址.
 * 在 `LogIn` 成功后可以订阅( `Subscribe` )合约, 接收行情. 最新行情可以由 `market_data` 函数获取.
 * 反之通过 `Unsubscribe` 取消订阅, 通过 `LogOut` 登出.
 */
class CTPMarketData : public CTPMarketDataBase {
public:
	CTPMarketData(const std::vector<IPAddress>& server_addr) : CTPMarketDataBase(server_addr) {}

protected:
	void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) override;

	MarketDepth CTPMarketData2MarketDepth(CThostFtdcDepthMarketDataField* pDepthMarketData);

private:
	std::map<std::string, Price> pre_latest_;
	std::map<std::string, Price> pre_high_;
	std::map<std::string, Price> pre_low_;
	std::map<std::string, Price> pre_turnover_;
	std::map<std::string, Volume> pre_volume_;
};

/// 可被观察的CTP行情基类
class ObservableCTPMarketDataBase : public CTPMarketDataBase, public Observable {
public:
	ObservableCTPMarketDataBase(const std::vector<IPAddress>& server_addr) : CTPMarketDataBase(server_addr){};
	ObservableCTPMarketDataBase(const IPAddress& server_addr) : CTPMarketDataBase(server_addr){};
};

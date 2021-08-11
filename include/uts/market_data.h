#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include <uts/data_struct.h>
#include <uts/observer.h>

/// 行情源基类. 行情相关类由此派生
class MarketDataSource {
public:
	/// 构造函数
	MarketDataSource(const std::vector<IPAddress>& server_addr) : server_addr_(server_addr) {}
	virtual ~MarketDataSource() = default;

	/// 是否已登录
	bool is_logged_in() const { return status_ == ConnectionStatus::Connected; }
	/// 已订阅合约
	std::set<Ticker> subscribed_tickers() const { return subscribed_tickers_; }
	/// 行情
	std::map<Ticker, MarketDepth>& market_data() { return market_data_; }

	/// 登录
	virtual void LogIn() = 0;
	/// 登出
	virtual void LogOut() = 0;

	/// 订阅合约
	virtual void Subscribe(const std::vector<Ticker>& ticker_list) = 0;
	/// 解除订阅
	virtual void Unsubscribe(const std::vector<Ticker>& ticker_list) = 0;

protected:
	std::vector<IPAddress> server_addr_;						///< 行情服务器地址
	ConnectionStatus status_ = ConnectionStatus::Disconnected;	///< 连接状态

	std::set<Ticker> subscribed_tickers_;		 ///< 已订阅合约
	std::map<Ticker, MarketDepth> market_data_;	 ///< 最新行情
};

///	可被观察的行情源
class ObservableMarketDataSource : public MarketDataSource, public Observable {
	/// 构造函数
	ObservableMarketDataSource(const std::vector<IPAddress>& server_addr) : MarketDataSource(server_addr) {}
	virtual ~ObservableMarketDataSource() = default;
};

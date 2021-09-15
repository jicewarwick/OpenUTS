#pragma once

#include <filesystem>
#include <map>
#include <vector>

#include <CTP/ThostFtdcUserApiStruct.h>
#include <nlohmann/json.hpp>
#include <uts/data_struct.h>
#include <uts/dbconfig.h>
#include <uts/market_data.h>
#include <uts/tradingaccount.h>

/// UnifiedTradingSystem
class UnifiedTradingSystem {
public:
	UnifiedTradingSystem();
	~UnifiedTradingSystem();

	// getter
	/// 注册账户是否为空
	bool empty() const;
	/// 注册账户数
	size_t size() const;
	/// 返回已注册账户索引
	std::vector<Account> available_accounts() const;
	/// 返回账户指针
	TradingAccount* GetHandle(const Account& account);

	// accounts
	/// 添加经纪商信息
	void AddBroker(const BrokerInfo& broker_info);
	void AddBroker(const std::vector<BrokerInfo>& broker_info);

	/// 添加账户信息
	void AddAccount(const std::vector<AccountInfo>& accounts_info);
	void AddAccount(const AccountInfo& account_info);

	/// 添加行情源信息
	void AddMarketDataSource(const std::vector<IPAddress>& server_addr);

	/// 设置不平今合约
	void setNoCloseTodayTickers(const std::set<Ticker>& no_close_today_tickers);

	[[deprecated]] void InitFromJsonConfig(nlohmann::json config);
	void InitFromDBConfig(const UTSConfigDB& config);

	/**
	 * @brief 返回合约基本信息
	 * @return 合约基本信息
	 * @pre 需在(至少一个)账户登录后调用`QueryInstruments`后调用才会有结果.
	 */
	std::map<Ticker, InstrumentInfo> instrument_info() const { return instrument_info_; }

	std::map<Account, std::map<InstrumentIndex, HoldingRecord>> GetHolding() const;
	std::map<Account, std::vector<TradingRecord>> GetTrades() const;
	std::map<Account, std::vector<OrderRecord>> GetOrders() const;
	std::map<InstrumentIndex, HoldingRecord> GetHolding(const Account& account) const;
	std::vector<TradingRecord> GetTrades(const Account& account) const;
	std::vector<OrderRecord> GetOrders(const Account& account) const;

	// login and logout
	void LogOn();
	void LogOn(const Account&);
	void LogOff();
	void LogOff(const Account&);

	// queries
	void QueryInstruments();
	void QueryCommissionRate();
	/// 订阅市场上的所有合约
	void SubscribeInstruments();
	/// 订阅指定合约
	void SubscribeInstruments(const std::vector<Ticker>& tickers);
	/// 筛选指定品种的所有合约
	std::vector<Ticker> ListProducts(std::vector<ProductID> product_ids);
	/// 订阅指定品种的所有合约
	void SubscribeProducts(const std::vector<ProductID>& product_ids);

	// place orders
	/// Async下单
	void PlaceOrderAsync(Order);
	/// 下单
	void PlaceOrderSync(Order);

	std::vector<Order> ProcessAdvancedOrder(Order order);
	void PlaceAdvancedOrderSync(Order);
	void PlaceAdvancedOrderAsync(Order);

	void CancelOrder(Account, OrderIndex);

	// batch actions
	void ClearAllHodlings(Account, TimeInForce time_in_force = TimeInForce::GFD,
						  OrderPriceType price_type = OrderPriceType::BestPrice);
	void CancelAllPendingOrders(Account);
	void CancelAllPendingOrders();

	void DumpInfoJson(const std::filesystem::path& loc) const;

private:
	std::map<Account, TradingAccount*> accounts_;
	std::map<BrokerName, BrokerInfo> broker_info_;

	std::map<Ticker, InstrumentInfo> instrument_info_;
	MarketDataSource* market_data_source_ = nullptr;
	std::map<Ticker, MarketDepth>* market_data_ = nullptr;
	std::set<Ticker> no_close_today_tickers_;

	// helper func
	TradingAccount* CheckAccount(const Account&) const;

	std::vector<Order> ReversePosition(HoldingRecord rec);
};

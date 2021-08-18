#pragma once

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <thread>

#include <CTP/ThostFtdcTraderApi.h>
#include <CTP/ThostFtdcUserApiDataType.h>
#include <uts/ratethrottler.h>
#include <uts/tradingaccount.h>

/**
 * @brief CTP 交易接口
 *
 * 提供账户的 资金, 持仓, 成交, 委托, 合约, 手续费的查询, 提供 CTP下单 接口
 */
class CTPTradingAccount : public TradingAccount, private CThostFtdcTraderSpi {
public:
	CTPTradingAccount(const AccountInfo& account_info, const BrokerInfo& ctp_broker_info);
	CTPTradingAccount(const CTPTradingAccount&) = delete;
	CTPTradingAccount& operator=(const CTPTradingAccount&) = delete;
	~CTPTradingAccount() override;

	virtual CapitalInfo Capital() const override;
	virtual const std::map<InstrumentIndex, HoldingRecord> holding() const override;
	virtual const std::vector<TradingRecord> trades() const override;
	virtual const std::map<OrderIndex, OrderRecord> orders() const override;

	// login
	void LogOnSync() override;
	void LogOnAsync() noexcept override;

	// log out
	void LogOffSync() noexcept override;
	void LogOffAsync() noexcept override;

	void QueryCapitalSync() override;
	// change password
	bool UpdatePassword(const Password& new_password) override;

	// query market info
	std::map<Ticker, InstrumentInfo> QueryInstruments() override;
	std::map<Ticker, InstrumentCommissionRate> QueryCommissionRate() override;
	InstrumentCommissionRate QueryCommissionRate(const Ticker&, InstrumentType) override;

	// order
	OrderIndex PlaceOrderASync(Order) override;
	void PlaceOrderSync(Order) override;
	std::vector<OrderIndex> BatchOrderSync(std::vector<Order> orders) override;
	std::map<OrderIndex, OrderStatus> GetBatchOrderStatus(std::vector<OrderIndex> indexes);
	std::vector<OrderIndex> GetUnfinishedOrder(std::vector<OrderIndex> indexes);

	// cancel order
	void CancelOrder(OrderIndex) override;
	void CancelAllPendingOrders() override;

	// IO
	nlohmann::json CurrentInfoJson() const override;

protected:
	void OnFrontConnected() override;
	void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo,
						   int nRequestID, bool bIsLast) override;
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID,
						bool bIsLast) override;
	void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
									CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField*, CThostFtdcRspInfoField*, int, bool) override;
	void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID,
						 bool bIsLast) override;
	void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID,
							bool bIsLast) override;
	void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate,
										  CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	void OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField* pOptionInstrCommRate,
									 CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo,
								  int nRequestID, bool bIsLast) override;
	void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID,
						  bool bIsLast) override;
	void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo,
								int nRequestID, bool bIsLast) override;
	void OnRtnTrade(CThostFtdcTradeField* pTrade) override;
	void OnRtnOrder(CThostFtdcOrderField* pOrder) override;

private:
	std::vector<IPAddress> trade_server_addr_;
	BrokerID broker_id_;
	UserProductInfo user_product_info_;
	AuthCode auth_code_;
	AppID app_id_;
	std::filesystem::path cache_path_;
	RateThrottler<std::chrono::seconds> rate_throttler_{1, std::chrono::seconds(1)};

	FrontID front_id_;
	SessionID session_id_;
	std::atomic_int request_id_ = 0;
	std::atomic<OrderRef> order_ref_;

	std::map<Ticker, InstrumentInfo> instrument_info_;
	std::map<Ticker, InstrumentCommissionRate> instrument_commission_rate_;

	std::mutex log_on_cv_mutex_;
	std::condition_variable log_on_cv_;

	std::mutex query_mutex_;
	mutable std::mutex capital_mutex_;
	mutable std::mutex holding_mutex_;
	mutable std::mutex trades_mutex_;
	mutable std::mutex order_mutex_;
	std::condition_variable query_cv_;
	std::thread capital_querying_thread_;

	std::mutex order_sync_mutex_;
	std::condition_variable order_cv_;

	CThostFtdcTraderApi* papi_ = nullptr;
	std::set<OrderIndex> cancelable_orders_;

	// query
	void TestQueryRequestsPerSecond();
	void QueryCapitalAsync();
	void QueryPreHolding();

	OrderIndex PlaceOrderASync(CThostFtdcInputOrderField&);
	void FilterCancelableOrders(std::map<OrderIndex, OrderRecord>::iterator& loc);
	void PostingLoginRequest();

	// translation
	CThostFtdcInputOrderField NativeOrder2CTPOrder(const Order& order) const;

	enum class LoggingError {
		NoError,
		FirstLoginPasswordNeedChangeError,
		WrongUsernamePasswordError,
		WeakPasswordError,
		PasswordOutofDateError,
		IPLimitedError,
		IPBanedError,
		UnknownLoggingError
	};
	LoggingError loggin_error_{LoggingError::NoError};
	bool success_ = false;
};

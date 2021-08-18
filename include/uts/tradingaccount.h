#pragma once

#include <map>
#include <vector>

#include <nlohmann/json.hpp>
#include <uts/data_struct.h>

/**
 * @brief 交易账户基类, 所有交易相关的类由此派生
 */
class TradingAccount {
public:
	/**
	 * @brief 构造函数. 初始化账户信息. 包括用户名, 经纪商名称, 账号和密码
	 * @param account_info 账户信息
	 */
	TradingAccount(const AccountInfo& account_info);
	TradingAccount(const TradingAccount&) = delete;
	TradingAccount& operator=(const TradingAccount&) = delete;
	virtual ~TradingAccount() = default;

	// getter
	/// 返回是否已登录
	bool is_logged_in() const;
	/// 返回账户名称
	AccountName account_name() const { return account_name_; }
	/// 返回经纪商名称
	BrokerName broker_name() const { return broker_name_; }
	/// 返回账户ID. 默认为 "用户名 - 经纪商名称"
	ID id() const { return id_; }
	/// 返回账户权益
	virtual CapitalInfo Capital() const = 0;
	/// 返回持仓记录
	virtual const std::map<InstrumentIndex, HoldingRecord> holding() const = 0;
	/// 返回成交记录
	virtual const std::vector<TradingRecord> trades() const = 0;
	/// 返回委托记录
	virtual const std::map<OrderIndex, OrderRecord> orders() const = 0;

	/// Async登录函数
	virtual void LogOnAsync() noexcept = 0;
	/// 登录函数
	virtual void LogOnSync() = 0;

	/// Async登出函数
	virtual void LogOffAsync() noexcept = 0;
	/// 登出函数
	virtual void LogOffSync() noexcept = 0;

	/// 刷新资金
	virtual void QueryCapitalSync() = 0;
	/// 修改密码
	virtual bool UpdatePassword(const Password& new_password) = 0;

	/// Async下单
	virtual OrderIndex PlaceOrderASync(Order) = 0;
	/// 下单函数
	virtual void PlaceOrderSync(Order) = 0;
	virtual std::vector<OrderIndex> BatchOrderSync(std::vector<Order> orders) = 0;
	/// 取消订单
	virtual void CancelOrder(OrderIndex) = 0;

	/// 取消所有未成交订单
	virtual void CancelAllPendingOrders() = 0;

	// querys
	/// 查询市场上的合约
	virtual std::map<Ticker, InstrumentInfo> QueryInstruments() = 0;
	/// 查询合约的手续费
	virtual std::map<Ticker, InstrumentCommissionRate> QueryCommissionRate() = 0;
	virtual InstrumentCommissionRate QueryCommissionRate(const Ticker&, InstrumentType) = 0;

	/// 查询净持仓
	virtual std::map<Ticker, int> GetNetHoldings() const;
	/// 查询净成交
	virtual std::map<Ticker, int> GetNetTrades() const;

	/// Json化账户数据
	virtual nlohmann::json CurrentInfoJson() const = 0;

protected:
	ConnectionStatus connection_status_ = ConnectionStatus::Uninitialized;	///< 连接状态

	const AccountName account_name_;		 ///< 账户名称
	const BrokerName broker_name_;			 ///< 经纪商名称
	const AccountNumberStr account_number_;	 ///< 账号
	Password password_;						 ///< 密码

	const ID id_;  ///< ID

	CapitalInfo capital_;								///< 账户权益
	std::map<InstrumentIndex, HoldingRecord> holding_;	///< 持仓记录
	std::vector<TradingRecord> trades_;					///< 成交记录
	std::map<OrderIndex, OrderRecord> orders_;			///< 委托记录
};

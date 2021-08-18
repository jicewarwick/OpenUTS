#pragma once

#include <sqlite3.h>

#include <chrono>
#include <filesystem>
#include <vector>
#include <set>

#include <uts/data_struct.h>

/**
 * @brief 配置信息. 保存为sqlite3格式. 主要包括服务器地址, 延迟等信息
 * @details 类构造时默认生成如下表: `broker_id` (期货公司brokerid), `cffex_contracts` ( `IF` 和 `IO` 合约),
 * `ctp_md_latency` ( `CTP` 行情服务器延迟), `ctp_md_server` ( `CTP` 行情服务器延迟), `ctp_trade_server` ( `CTP`
 * 交易服务器列表)
 */
class UTSConfigDB {
public:
	/**
	 * @brief 构造函数.
	 * @param path 数据库路径
	 * @exception DBFileError 数据库无法打开错误
	 */
	UTSConfigDB(const std::filesystem::path& db_name);
	UTSConfigDB(const UTSConfigDB&) = delete;
	UTSConfigDB& operator=(const UTSConfigDB&) = delete;
	UTSConfigDB(UTSConfigDB&& rhs) noexcept;
	~UTSConfigDB();

	// Server info
	/**
	 * @brief 获取未测速的 `CTP` 行情服务器列表
	 */
	std::vector<IPAddress> UnSpeedTestedCTPMDServers() const;
	/**
	 * @brief 获取延迟最低的 `n` 个 `CTP` 行情服务器地址
	 * @param n 服务器数量
	 */
	std::vector<IPAddress> FartestCTPMDServers(size_t n) const;
	/**
	 * @brief 添加 `CTP` 行情服务器的延迟信息
	 */
	void AppendSpeedTestResult(const IPAddress& addr, std::chrono::milliseconds ms);
	// std::vector<IPAddress> CTPMDServers();

	/// 获取订阅合约列表
	std::vector<Ticker> GetSubscriptionTickers() const;
	/// 获取不平今合约列表
	std::set<Ticker> GetNoCloseTodayContracts() const;

	/// 获取经纪商信息
	std::map<BrokerName, BrokerInfo> GetBrokerInfo() const;
	/// 获取交易账户
	std::vector<AccountInfo> GetAccountInfo() const;
	/// 获取MySQL数据库登录信息
	MySQLConnectionInfo GetMySQLConnectionInfo() const;

private:
	sqlite3* conn_ = nullptr;
};

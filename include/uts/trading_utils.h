#pragma once

#include <chrono>
#include <string>
#include <tuple>
#include <vector>

#include <uts/data_struct.h>

/**
 * @brief 生成随机交易订单
 * @param account_name 账户名称
 * @param broker_name 经纪商名称
 * @param tikcers 合约池
 * @param n 生成订单数量
 */
std::vector<Order> GenerateRandomOrders(const AccountName& account_name, const BrokerName& broker_name,
										const std::vector<Ticker>& tickers, int n);

/// 翻转多空方向
inline Direction ReverseDirection(Direction dir) { return static_cast<Direction>(-static_cast<int>(dir)); }

// 返回看涨期权对应的看跌期权
inline OptionTicker CallOptionToPutOption(const OptionTicker& call) {
	size_t pos = call.find_last_of("C");
	if ((pos == std::string::npos) || (pos <= 2)) { throw std::runtime_error("Invalid Option Ticker Input!"); }
	OptionTicker put = call;
	put[pos] = 'P';
	return put;
}

// 返回看跌期权对应的看涨期权
inline OptionTicker PutOptionToCallOption(const OptionTicker& put) {
	size_t pos = put.find_last_of("P");
	if ((pos == std::string::npos) || (pos <= 2)) { throw std::runtime_error("Invalid Option Ticker Input!"); }
	OptionTicker call = put;
	call[pos] = 'C';
	return call;
}

/// 股指期权交割月份
inline YearMonth StockIndexOptionYearMonth(const Ticker& ticker) { return ticker.substr(2, 4); }

/// 股指期权类型
inline OptionType StockIndexOptionType(const Ticker& ticker) {
	return ticker.substr(7, 1) == "C" ? OptionType::Call : OptionType::Put;
}

/// 股指期权行权价
inline Strike StockIndexOptionStrike(const Ticker& ticker) { return std::atoi(ticker.substr(9).c_str()); }

/// 股指期权信息
inline std::tuple<YearMonth, OptionType, Strike> StockIndexOptionInfo(const Ticker& ticker) {
	return {StockIndexOptionYearMonth(ticker), StockIndexOptionType(ticker), StockIndexOptionStrike(ticker)};
}

inline long HashOptionTicker(const OptionTicker& ticker) {
	long k = StockIndexOptionStrike(ticker);
	long month = atoi(StockIndexOptionYearMonth(ticker).c_str());
	return month * 10000 + k;
}
/**
 * @brief 生成最近的期货闭市时间
 */
std::chrono::time_point<std::chrono::system_clock> GetMarketCloseTime();

/**
 * @brief 解析格式为 `YYYYMMDD` 的时间戳
 * @param datestr 时间戳
 */
std::chrono::time_point<std::chrono::system_clock> ParseDate(std::string datestr);

/**
 * @brief 今天(含)到 datestr 的自然天数
 * @param datestr  格式为 `YYYYMMDD` 的时间戳
 */
int DaysLeft(DateStr datestr);

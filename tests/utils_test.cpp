#include "utils.h"

#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <uts/dbconfig.h>
#include <uts/trading_utils.h>

#include "data_struct.h"
#include "enum_utils.h"
#include "utsexceptions.h"

using namespace nlohmann;
using std::vector, std::map;

TEST(UtilsTest, ReverseDirection) {
	ASSERT_EQ(ReverseDirection(Direction::Long), Direction::Short);
	ASSERT_EQ(ReverseDirection(Direction::Short), Direction::Long);
}

TEST(UtilsTest, StockIndexOptionYearMonth) {
	ASSERT_EQ(StockIndexOptionYearMonth("IO2106-C-4100"), "2106");
	ASSERT_EQ(StockIndexOptionYearMonth("IO2106-P-4100"), "2106");
}

TEST(UtilsTest, StockIndexOptionType) {
	ASSERT_EQ(StockIndexOptionType("IO2106-C-4100"), OptionType::Call);
	ASSERT_EQ(StockIndexOptionType("IO2106-P-4100"), OptionType::Put);
}

TEST(UtilsTest, StockIndexOptionStrike) {
	ASSERT_EQ(StockIndexOptionStrike("IO2106-C-4100"), 4100);
	ASSERT_EQ(StockIndexOptionStrike("IO2106-P-4100"), 4100);
}

TEST(UtilsTest, HashOptionTicker) {
	ASSERT_EQ(HashOptionTicker("IO2106-C-4100"), 21064100);
	ASSERT_EQ(HashOptionTicker("IO2106-P-4100"), 21064100);
}

TEST(UtilsTest, PutCallSwitchTest) {
	ASSERT_EQ(CallOptionToPutOption("IO2106-C-4100"), "IO2106-P-4100");
	ASSERT_EQ(PutOptionToCallOption("IO2106-P-4100"), "IO2106-C-4100");
	ASSERT_THROW(CallOptionToPutOption("IO2106-P-4100"), std::runtime_error);
	ASSERT_THROW(CallOptionToPutOption("IC2106-P-4100"), std::runtime_error);
	ASSERT_THROW(PutOptionToCallOption("IP2106-C-4100"), std::runtime_error);
	ASSERT_THROW(PutOptionToCallOption("IO2106-"), std::runtime_error);
}

TEST(DBConfigTest, DBRuns) {
	UTSConfigDB conf("./test_files/sample_db.sqlite3");
	std::vector<IPAddress> t1 = conf.UnSpeedTestedCTPMDServers();
	std::vector<IPAddress> t2 = conf.FartestCTPMDServers(1);
	std::vector<Ticker> t3 = conf.GetSubscriptionTickers();
	// ASSERT_EQ(t1.size(), 0);
	ASSERT_EQ(t2.size(), 1);
	// ASSERT_GT(t3.size(), 10);

	std::map<BrokerName, BrokerInfo> broker_id_info = conf.GetBrokerInfo();
	ASSERT_EQ(broker_id_info.at("simnow").broker_id, "9999");

	std::vector<AccountInfo> account_info = conf.GetAccountInfo();
	ASSERT_EQ(account_info[0].account_name, "simnow_test");
	ASSERT_EQ(account_info[1].account_name, "simnow_test1");
	ASSERT_TRUE(account_info[0].enable);
	ASSERT_FALSE(account_info[1].enable);

	MySQLConnectionInfo conn_info = conf.GetMySQLConnectionInfo();
	ASSERT_EQ(conn_info.addr, "127.0.0.1");
	ASSERT_EQ(conn_info.db_name, "asharedata");

	std::set<Ticker> tickers = conf.GetNoCloseTodayContracts();
	ASSERT_EQ(tickers.size(), 1);
	ASSERT_TRUE(tickers.contains("AP110"));
}

TEST(JsonTest, FileNotExist) {
	std::filesystem::path login_info_path("test_files/login_info1.json");
	ASSERT_THROW(ReadJsonFile(login_info_path), IOError);
	try {
		ReadJsonFile(login_info_path);
	} catch (const std::exception& e) { std::cout << e.what() << std::endl; }
}

TEST(JsonTest, JsonFormatError) {
	std::filesystem::path login_info_path("test_files/login_info_format_error.json");
	ASSERT_THROW(ReadJsonFile(login_info_path), JsonFileError);
}

TEST(JsonTest, JsonReadFile) {
	std::filesystem::path login_info_path("test_files/simnow.json");
	json login_info_json = ReadJsonFile(login_info_path);
}

TEST(JsonTest, OrderJson) {
	std::filesystem::path order_json_path("test_files/market_orders.json");
	json order_json = ReadJsonFile(order_json_path);
	vector<Order> order = order_json.get<vector<Order>>();
}

TEST(JsonTest, ContractListJson) {
	std::filesystem::path order_json_path("test_files/contracts.json");
	json order_json = ReadJsonFile(order_json_path);
	map<Ticker, DateStr> order = order_json.get<map<Ticker, DateStr>>();
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

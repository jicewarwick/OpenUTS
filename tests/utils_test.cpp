#include "utils.h"

#include <filesystem>
#include <iostream>
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
	GTEST_ASSERT_EQ(ReverseDirection(Direction::Long), Direction::Short);
	GTEST_ASSERT_EQ(ReverseDirection(Direction::Short), Direction::Long);
}

TEST(UtilsTest, StockIndexOptionYearMonth) {
	GTEST_ASSERT_EQ(StockIndexOptionYearMonth("IO2106-C-4100"), "2106");
	GTEST_ASSERT_EQ(StockIndexOptionYearMonth("IO2106-P-4100"), "2106");
}

TEST(UtilsTest, StockIndexOptionType) {
	GTEST_ASSERT_EQ(StockIndexOptionType("IO2106-C-4100"), OptionType::Call);
	GTEST_ASSERT_EQ(StockIndexOptionType("IO2106-P-4100"), OptionType::Put);
}

TEST(UtilsTest, StockIndexOptionStrike) {
	GTEST_ASSERT_EQ(StockIndexOptionStrike("IO2106-C-4100"), 4100);
	GTEST_ASSERT_EQ(StockIndexOptionStrike("IO2106-P-4100"), 4100);
}

TEST(UtilsTest, HashOptionTicker) {
	GTEST_ASSERT_EQ(HashOptionTicker("IO2106-C-4100"), 21064100);
	GTEST_ASSERT_EQ(HashOptionTicker("IO2106-P-4100"), 21064100);
}

TEST(UtilsTest, PutCallSwitchTest) {
	GTEST_ASSERT_EQ(CallOptionToPutOption("IO2106-C-4100"), "IO2106-P-4100");
	GTEST_ASSERT_EQ(PutOptionToCallOption("IO2106-P-4100"), "IO2106-C-4100");
	ASSERT_THROW(CallOptionToPutOption("IO2106-P-4100"), std::runtime_error);
	ASSERT_THROW(CallOptionToPutOption("IC2106-P-4100"), std::runtime_error);
	ASSERT_THROW(PutOptionToCallOption("IP2106-C-4100"), std::runtime_error);
	ASSERT_THROW(PutOptionToCallOption("IO2106-"), std::runtime_error);
}

TEST(DBConfigTest, DBRuns) {
	UTSConfigDB conf("./test_files/db.sqlite3");
	std::vector<IPAddress> t1 = conf.UnSpeedTestedCTPMDServers();
	std::vector<IPAddress> t2 = conf.FartestCTPMDServers(10);
	std::vector<Ticker> t3 = conf.CurrentIFIOTickers();

	GTEST_ASSERT_EQ(t1.size(), 0);
	GTEST_ASSERT_EQ(t2.size(), 10);
	GTEST_ASSERT_GT(t3.size(), 10);
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

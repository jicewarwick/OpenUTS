#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "ctptradingaccount.h"
#include "data_struct.h"
#include "enum_utils.h"
#include "utils.h"
#include "utsexceptions.h"

using std::cout, std::endl, std::filesystem::path, std::map, nlohmann::json, std::vector, std::string;
using namespace std::chrono_literals;

TEST(CTPTradingAccountLoginTest, WrongPasswordTest) {
	path login_info_path("test_files/simnow.json");
	json info = ReadJsonFile(login_info_path);
	AccountInfo account_info = info.at("accounts").get<vector<AccountInfo>>()[0];
	BrokerInfo broker_info = info.at("brokers").get<vector<BrokerInfo>>()[1];
	account_info.password = "112233";
	map<Ticker, InstrumentInfo> instrument_info;
	map<Ticker, MarketDepth> market_data;
	CTPTradingAccount account(account_info, broker_info);
	ASSERT_THROW(account.LogOnSync(), AccountNumberPasswordError);
}

class CTPTradingAccountTest : public ::testing::Test {
protected:
	static CTPTradingAccount* account_;

	static void SetUpTestCase() {
		spdlog::set_level(spdlog::level::debug);
		json info = ReadJsonFile("test_files/simnow.json");
		AccountInfo account_info = info.at("accounts").get<vector<AccountInfo>>()[0];
		BrokerInfo broker_info = info.at("brokers").get<vector<BrokerInfo>>()[1];
		map<Ticker, InstrumentInfo> instrument_info;
		map<Ticker, MarketDepth> market_data;
		account_ = new CTPTradingAccount(account_info, broker_info);

		account_->LogOnSync();
	}

	static void TearDownTestCase() {
		account_->LogOffSync();
		delete account_;
	}
};

CTPTradingAccount* CTPTradingAccountTest::account_ = nullptr;

TEST_F(CTPTradingAccountTest, InfoTest) {
	auto res = account_->CurrentInfoJson().dump(4);
	cout << "CurrentInfo Test Results: " << endl;
	cout << res << endl;
	cout << "--------------------------" << endl;
}

TEST_F(CTPTradingAccountTest, NetHoldingTest) {
	auto res = static_cast<json>(account_->GetNetHoldings()).dump(4);
	cout << "Net Holding Test Results: " << endl;
	cout << res << endl;
	cout << "--------------------------" << endl;
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

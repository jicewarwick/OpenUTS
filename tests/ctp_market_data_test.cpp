#include <vector>

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "ctpmarketdata.h"
#include "utils.h"

using nlohmann::json;

class CTPMarketDataTest : public ::testing::Test {
protected:
	static CTPMarketData* md_;

	static void SetUpTestCase() {
		json login_info_json = ReadJsonFile("test_files/simnow.json");
		auto market_server = login_info_json.at("md_server").get<std::map<ServerName, IPAddress>>();
		std::vector<std::string> md_server = MapValues(market_server);

		md_ = new CTPMarketData(md_server);
		md_->LogIn();
	}

	static void TearDownTestCase() {
		md_->LogOut();
		delete md_;
	}
};

CTPMarketData* CTPMarketDataTest::md_ = nullptr;

TEST_F(CTPMarketDataTest, MarketDataSubscribeTest) {
	std::vector<Ticker> test_products{"ag2009", "AP012", "c2009", "IC2009"};
	md_->Subscribe(test_products);
	GTEST_ASSERT_EQ(md_->subscribed_tickers().size(), 4);
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

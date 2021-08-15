#include <chrono>
#include <iostream>
#include <map>
#include <thread>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "ctptradingaccount.h"
#include "enum_utils.h"
#include "unifiedtradingsystem.h"
#include "utils.h"
#include "utsexceptions.h"

using nlohmann::json, std::vector, std::string, std::map, std::cout, std::endl;
using namespace std::chrono_literals;

class TradingSystemTest : public ::testing::Test {
protected:
	static UnifiedTradingSystem* uts_;
	static std::map<Ticker, InstrumentInfo> instrument_info_;
	const static vector<string> test_products;
	static vector<std::pair<Ticker, Exchange>> testing_instrument_;

	vector<Order> OrderTestTemplate(string file_name);

	static void SetUpTestCase() {
		json info = ReadJsonFile("test_files/simnow.json");
		uts_ = new UnifiedTradingSystem();
		uts_->InitFromJsonConfig(info);
		uts_->LogOn();
		uts_->QueryInstruments();
		uts_->SubscribeInstruments();
		instrument_info_ = uts_->instrument_info();
		for (const auto& it : test_products) {
			auto loc = std::find_if(instrument_info_.begin(), instrument_info_.end(),
									[&](auto input) { return input.second.product_id == it; });
			testing_instrument_.emplace_back(loc->second.instrument_id, loc->second.exchange);
		}
	}

	static void TearDownTestCase() {
		uts_->LogOff();
		delete uts_;
	}
};

UnifiedTradingSystem* TradingSystemTest::uts_ = nullptr;
std::map<Ticker, InstrumentInfo> TradingSystemTest::instrument_info_ = {};
const vector<string> TradingSystemTest::test_products{"ag", "AP", "c", "IC"};
vector<std::pair<Ticker, Exchange>> TradingSystemTest::testing_instrument_;

void AssureTradingBroker(std::string& broker_name) {
	if (broker_name.starts_with("simnow")) {
		cout << "Testing broker must be simnow!!!" << endl;
		std::terminate();
	};
}

vector<Order> TradingSystemTest::OrderTestTemplate(string file_name) {
	json orders_json = ReadJsonFile(file_name);
	Order order = orders_json.get<vector<Order>>()[0];

	auto account = uts_->available_accounts()[0];
	AssureTradingBroker(account.second);

	vector<Order> orders;
	for (const auto& it : test_products) {
		order.account_name = account.first;
		order.broker_name = account.second;
		auto loc = std::find_if(instrument_info_.begin(), instrument_info_.end(),
								[&](auto input) { return input.second.product_id == it; });
		order.instrument_id = loc->second.instrument_id;
		order.exchange = loc->second.exchange;
		orders.push_back(order);
	}

	return orders;
}

TEST_F(TradingSystemTest, GetterTest) {
	std::cout << "Holdings: " << std::endl;
	std::cout << static_cast<json>(uts_->GetHolding()).dump(4) << std::endl;

	std::cout << "Trades: " << std::endl;
	std::cout << static_cast<json>(uts_->GetTrades()).dump(4) << std::endl;

	std::cout << "Orders: " << std::endl;
	std::cout << static_cast<json>(uts_->GetOrders()).dump(4) << std::endl;
}

TEST_F(TradingSystemTest, CancelAllOrderTest) {
	auto account = uts_->available_accounts()[0];
	AssureTradingBroker(account.second);
	uts_->CancelAllPendingOrders(account);
	std::this_thread::sleep_for(2s);
}

TEST_F(TradingSystemTest, OrderErrorTest) {
	json orders_json = ReadJsonFile("test_files/fak_orders.json");
	Order order = orders_json.get<vector<Order>>()[0];

	auto account = uts_->available_accounts()[0];
	AssureTradingBroker(account.second);
	order.account_name = account.first;
	order.broker_name = account.second;
	order.instrument_id = testing_instrument_[0].first;
	ASSERT_NO_THROW(uts_->ProcessAdvancedOrder(order));
	// no instrument
	Order wrong_instrument_order = order;
	wrong_instrument_order.instrument_id = "aa";
	EXPECT_THROW(uts_->ProcessAdvancedOrder(wrong_instrument_order), OrderInfoError);
	// wrong volume
	Order wrong_volume_order = order;
	wrong_volume_order.volume = 0;
	EXPECT_THROW(uts_->ProcessAdvancedOrder(wrong_volume_order), OrderInfoError);
	// wrong open_close
	Order wrong_open_close_order = order;
	wrong_open_close_order.open_close = OpenCloseType::ForceOff;
	EXPECT_THROW(uts_->ProcessAdvancedOrder(wrong_open_close_order), OrderInfoError);
	wrong_open_close_order.open_close = OpenCloseType::ForceClose;
	EXPECT_THROW(uts_->ProcessAdvancedOrder(wrong_open_close_order), OrderInfoError);
	wrong_open_close_order.open_close = OpenCloseType::LocalForceOff;
	EXPECT_THROW(uts_->ProcessAdvancedOrder(wrong_open_close_order), OrderInfoError);
	// wrong price
	// Order wrong_price_order = order;
	// order.order_price_type = OrderPriceType::LimitPrice;
	// wrong_price_order.limit_price = 1234.56789;
	// EXPECT_THROW(uts_->ProcessAdvancedOrder(wrong_price_order), OrderInfoError);
}
TEST_F(TradingSystemTest, MarketOrderTest) {
	vector<Order> orders = OrderTestTemplate("test_files/market_orders.json");
	for (auto& adv_order : orders) { uts_->PlaceAdvancedOrderAsync(adv_order); }
}

TEST_F(TradingSystemTest, FAKOrderTest) {
	vector<Order> orders = OrderTestTemplate("test_files/fak_orders.json");
	for (auto& adv_order : orders) { uts_->PlaceAdvancedOrderAsync(adv_order); }
}

TEST_F(TradingSystemTest, AskPriceLimitOrderTest) {
	vector<Order> orders = OrderTestTemplate("test_files/ask_price_limit_order.json");
	for (auto& adv_order : orders) { uts_->PlaceAdvancedOrderAsync(adv_order); }
}

TEST_F(TradingSystemTest, BidPriceLimitOrderTest) {
	vector<Order> orders = OrderTestTemplate("test_files/bid_price_limit_order.json");
	for (auto& adv_order : orders) { uts_->PlaceAdvancedOrderAsync(adv_order); }
}

TEST_F(TradingSystemTest, BestPriceLimitOrderTest) {
	vector<Order> orders = OrderTestTemplate("test_files/best_price_limit_order.json");
	for (auto& adv_order : orders) { uts_->PlaceAdvancedOrderAsync(adv_order); }
}

TEST_F(TradingSystemTest, ClearAllHoldingsTest) {
	auto account = uts_->available_accounts()[0];
	uts_->ClearAllHodlings(account, TimeInForce::GFD, OrderPriceType::BestPrice);
	std::this_thread::sleep_for(2s);
	ASSERT_EQ(uts_->GetHolding(account).size(), 0);
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

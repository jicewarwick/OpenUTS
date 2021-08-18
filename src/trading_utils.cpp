#include "trading_utils.h"

#include <chrono>
#include <iomanip>
#include <random>
#include <string>
#include <vector>

#include "utsexceptions.h"

using std::string, std::vector;

vector<Order> GenerateRandomOrders(const AccountName& account_name, const BrokerName& broker_name,
								   const vector<Ticker>& tickers, int n) {
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> direction_dist(0, 1);
	std::uniform_int_distribution<int> instrument_dist(0, static_cast<int>(tickers.size() - 1));

	vector<Order> ret;
	for (int i = 0; i < n; ++i) {
		Order tmp{.account_name = account_name,
				  .broker_name = broker_name,
				  .instrument_id = tickers[instrument_dist(mt)],
				  .time_in_force = TimeInForce::GFD,
				  .open_close = OpenCloseType::Open,
				  .hedge_flag = HedgeFlagType::Speculation,
				  .direction = direction_dist(mt) ? Direction::Long : Direction::Short,
				  .volume = 1,
				  .order_price_type = OrderPriceType::LastPrice,
				  .tick_offset = 2,
				  .contingent_condition = OrderContingentCondition::Immediately};
		ret.push_back(std::move(tmp));
	}
	return ret;
}

std::chrono::time_point<std::chrono::system_clock> GetMarketCloseTime() {
	using namespace std::chrono;
	auto tp = system_clock::now();
	auto today = floor<days>(tp);
	int diff = tp > today + hours(17) ? 26 : 15;
	return today + hours(diff - 8);	 // china's time zone difference
}

std::chrono::time_point<std::chrono::system_clock> ParseDate(std::string datestr) {
	std::tm t = {};
	std::istringstream ss(datestr);
	ss.imbue(std::locale("zh_CN.utf-8"));
	ss >> std::get_time(&t, "%Y%m%d");
	if (ss.fail()) {
		throw ConfigError();
	} else {
		return std::chrono::system_clock::from_time_t(std::mktime(&t));
	}
}

int DaysLeft(DateStr datestr) {
	auto date = ParseDate(datestr);
	std::chrono::duration diff = date - std::chrono::system_clock::now();
	return std::chrono::ceil<std::chrono::days>(diff).count() + 1;
}

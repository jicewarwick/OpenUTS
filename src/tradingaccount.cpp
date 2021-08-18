#include "tradingaccount.h"

using std::map, std::string;

TradingAccount::TradingAccount(const AccountInfo& account_info)
	: account_name_(account_info.account_name), broker_name_(account_info.broker_name),
	  account_number_(account_info.account_number), password_(account_info.password),
	  id_(account_name_ + " - " + broker_name_) {}

bool TradingAccount::is_logged_in() const { return connection_status_ == ConnectionStatus::Done; }

map<string, int> TradingAccount::GetNetHoldings() const {
	map<string, int> ret;
	for (auto& [index, record] : holding_) {
		ret[index.instrument_id] += record.total_quantity * static_cast<int>(index.direction);
	}
	return ret;
}

map<string, int> TradingAccount::GetNetTrades() const {
	map<string, int> ret;
	for (const auto& record : trades_) {
		ret[record.instrument_id] += record.volume * static_cast<int>(record.direction);
	}
	return ret;
}

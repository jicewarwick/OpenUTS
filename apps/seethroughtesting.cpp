#include <chrono>
#include <thread>
#include <vector>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "trading_utils.h"
#include "unifiedtradingsystem.h"
#include "utsexceptions.h"

int main(int argc, char* argv[]) {
	std::filesystem::path config_file;

	CLI::App app{"Seethrough Testing routines"};
	app.add_option("-c,--config", config_file, "UTS config db location")->required()->check(CLI::ExistingFile);
	CLI11_PARSE(app, argc, argv)

	UTSConfigDB db(config_file);
	UnifiedTradingSystem uts{};
	try {
		uts.InitFromDBConfig(db);
	} catch (const std::exception& e) {
		spdlog::error("{}", e.what());
		spdlog::error("Error init from sqlite3 database. Please recheck. Exiting.");
		return EXIT_FAILURE;
	}

	auto accounts = uts.available_accounts();
	const Account& account = accounts[0];
	AccountName account_name = account.first;
	BrokerName broker_name = account.second;

	if (!account_name.ends_with("test")) { throw std::runtime_error("testing account name must end with test!"); }

	uts.LogIn();
	uts.QueryInstruments();
	uts.SubscribeInstruments();
	std::this_thread::sleep_for(std::chrono::seconds(1));

	std::vector<Ticker> tickers;
	for (auto& [ticker, info] : uts.instrument_info()) {
		if (info.exchange == Exchange::SHF) {
			tickers.push_back(info.instrument_id);
			spdlog::trace("{} added to the list.", ticker);
		}
	}
	auto orders = GenerateRandomOrders(account_name, broker_name, tickers, 3);

	for (const auto& order : orders) { uts.PlaceAdvancedOrderSync(order); }

	std::this_thread::sleep_for(std::chrono::seconds(5));
	uts.ClearAllHodlings(account);

	uts.DumpInfoJson("res.json");
}

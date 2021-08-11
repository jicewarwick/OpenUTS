#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "trading_utils.h"
#include "unifiedtradingsystem.h"
#include "utils.h"
#include "utsexceptions.h"

using std::string, nlohmann::json, std::vector;

int main(int argc, char* argv[]) {
	std::filesystem::path config_file;

	CLI::App app{"Seethrough Testing routines"};
	app.add_option("-c,--config", config_file, "json config file to login into CTP account")
		->required()
		->check(CLI::ExistingFile);
	CLI11_PARSE(app, argc, argv);

	json config;
	try {
		config = ReadJsonFile(config_file);
	} catch (std::exception& e) {
		spdlog::error(e.what());
		return EXIT_FAILURE;
	}

	UnifiedTradingSystem uts{};
	try {
		uts.InitFromJsonConfig(config);
	} catch (...) {
		spdlog::error("Info in {} is imcomplete or wrong. Please recheck. Exiting.", config_file.string().c_str());
		return EXIT_FAILURE;
	}

	auto accounts = uts.available_accounts();
	const Account& account = accounts[0];
	AccountName account_name = account.first;
	BrokerName broker_name = account.second;

	try {
		uts.LogOn();
	} catch (FirstLoginPasswordNeedChangeError& e) {
		if (config.count("new_password")) {
			string new_password = config.at("new_password").get<string>();
			if (uts.GetHandle(account)->UpdatePassword(new_password)) {
				uts.LogOn();
			} else {
				throw std::move(e);
			}
		} else {
			throw std::move(e);
		}
	}
	uts.QueryInstruments();
	uts.SubscribeInstruments();
	std::this_thread::sleep_for(std::chrono::seconds(1));

	vector<Ticker> tickers;
	for (auto& [ticker, info] : uts.instrument_info()) {
		if (info.exchange_id == "SHFE") {
			tickers.push_back(info.instrument_id);
			spdlog::trace("{} added to the list.", ticker);
		}
	}
	auto orders = GenerateRandomOrders(account_name, broker_name, tickers, 3);

	for (const auto& order : orders) { uts.PlaceAdvancedOrderSync(order); }
	// TradingAccount* handle = uts.GetHandle(account);
	// handle->BatchOrderSync(orders);

	std::this_thread::sleep_for(std::chrono::seconds(5));
	uts.ClearAllHodlings(account);

	uts.DumpInfoJson("res.json");
}

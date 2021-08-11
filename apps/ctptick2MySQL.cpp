#include <algorithm>
#include <chrono>
#include <map>
#include <ranges>
#include <vector>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "ctpmarketdatarecorder.h"
#include "enum_utils.h"
#include "mariadbdatarecorder.h"
#include "trading_utils.h"
#include "unifiedtradingsystem.h"
#include "utils.h"

using nlohmann::json, std::string, std::vector;
using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
	std::filesystem::path config_file;
	std::filesystem::path contracts_file;

	CLI::App app{"Dump CTP tick data into mariadb"};
	app.add_option("-c,--config", config_file, "json config file on info about CTP market server")
		->required()
		->check(CLI::ExistingFile);
	app.add_option("--contracts", contracts_file, "json file that lists contracts to record");

	CLI11_PARSE(app, argc, argv);

	json config;
	vector<Ticker> tickers;
	try {
		config = ReadJsonFile(config_file);
		if (contracts_file != "") { tickers = ReadJsonFile(contracts_file).get<vector<Ticker>>(); }
	} catch (std::exception& e) {
		spdlog::error(e.what());
		return EXIT_FAILURE;
	}

	auto market_server = config.at("md_server").get<std::map<ServerName, IPAddress>>();
	vector<string> md_server = MapValues(market_server);
	MySQLConnectionInfo db_info = config.at("mysql").get<MySQLConnectionInfo>();

	if (contracts_file == "") {
		spdlog::info("Getting contracts from trading API.");
		{
			UnifiedTradingSystem uts{};
			try {
				uts.InitFromJsonConfig(config);
			} catch (...) {
				spdlog::error("Info in {} is imcomplete or wrong. Please recheck. Exiting.",
							  config_file.string().c_str());
				return EXIT_FAILURE;
			}

			uts.LogOn();
			std::this_thread::sleep_for(2s);
			uts.QueryInstruments();
			tickers = uts.ListProducts({"IF", "IO"});
		}
		std::this_thread::sleep_for(5s);
	}
	MariadbDataRecorder sink(db_info);
	CTPMarketDataRecorder market_data_recorder(md_server);
	market_data_recorder.setSink(&sink);
	market_data_recorder.LogIn();
	market_data_recorder.Subscribe(tickers);

	auto end_time = GetMarketCloseTime() + 2min;
	// TODO: C++20 fmt replace
#include <ctime>
	auto end_time_t = std::chrono::system_clock::to_time_t(end_time);
	spdlog::info("Program ends at {}", ctime(&end_time_t));
	// spdlog::info(std::format("Program ends at {:%c}", end_time));

	std::this_thread::sleep_until(end_time);
	// std::this_thread::sleep_for(1min);
}

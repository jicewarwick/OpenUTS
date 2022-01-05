#include <chrono>
#include <filesystem>
#include <thread>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "dbconfig.h"
#include "trading_utils.h"
#include "unifiedtradingsystem.h"

using std::string;

int main(int argc, char* argv[]) {
	std::filesystem::path config_file;
	int interval = 60;

	CLI::App app{"Deamon that logs multiple ctp account info which includes capital, holdings, trades, orders, etc."};
	app.add_option("-c,--config", config_file, "UTS config db location")->required()->check(CLI::ExistingFile);
	app.add_option("-n,--interval", interval, "Interval in seconds to dump info to file");
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

	uts.LogIn();
	uts.QueryInstruments();
	uts.SubscribeInstruments();

	auto end_time = GetMarketCloseTime() + std::chrono::minutes(5);

	// TODO: C++20 fmt replace
#include <ctime>
#include <sstream>
	auto tt = std::chrono::system_clock::to_time_t(end_time);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&tt), "%Y-%m-%d");
	string output_json_name = ss.str() + "_ctp.json";
	// string output_json_name = std::format("{:%F}_ctp.json", end_time);

	do {
		std::this_thread::sleep_for(std::chrono::seconds(interval));
		uts.DumpInfoJson(output_json_name);
	} while (std::chrono::system_clock::now() < end_time);
}

#include <chrono>
#include <filesystem>
#include <thread>
#include <vector>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "trading_utils.h"
#include "unifiedtradingsystem.h"
#include "utils.h"

using nlohmann::json, std::vector, std::string;

int main(int argc, char* argv[]) {
	std::filesystem::path config_file;

	CLI::App app{"Deamon that logs multiple ctp account info which includes capital, holdings, trades, orders, etc."};
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
	int interval;
	try {
		interval = config.at("update_interval_in_seconds").get<int>();
		uts.InitFromJsonConfig(config);
	} catch (...) {
		spdlog::error("Info in {} is imcomplete or wrong. Please recheck. Exiting.", config_file.string().c_str());
		return EXIT_FAILURE;
	}

	uts.LogOn();
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

#include <filesystem>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "unifiedtradingsystem.h"
#include "utils.h"

using nlohmann::json;

int main(int argc, char* argv[]) {
	std::filesystem::path config_file;
	std::filesystem::path output_json_file = "commission_rate.json";

	CLI::App app{"Query Future Commission Rate"};
	app.add_option("-c,--config", config_file, "json config file to login into CTP account")
		->required()
		->check(CLI::ExistingFile);
	app.add_option("-o,--output", output_json_file, "path to write future commission info");

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

	uts.LogOn();
	uts.QueryInstruments();
	uts.QueryCommissionRate();
	uts.DumpInfoJson(output_json_file);
}

#include <filesystem>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "dbconfig.h"
#include "unifiedtradingsystem.h"

int main(int argc, char* argv[]) {
	std::filesystem::path config_file;
	std::filesystem::path output_json_file = "commission_rate.json";

	CLI::App app{"Query Future Commission Rate"};
	app.add_option("-c,--config", config_file, "UTS config db location")->required()->check(CLI::ExistingFile);
	app.add_option("-o,--output", output_json_file, "path to write future commission info");
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

	uts.LogOn();
	uts.QueryCommissionRate();
	uts.DumpInfoJson(output_json_file);
}

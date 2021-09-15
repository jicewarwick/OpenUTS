#include <chrono>
#include <thread>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "dbconfig.h"
#include "trading_utils.h"
#include "unifiedtradingsystem.h"
#include "utsexceptions.h"

using std::string;

int main(int argc, char* argv[]) {
	std::filesystem::path config_file;
	std::string new_password;

	CLI::App app{"Change active account to new password. Currently only support one active account."};
	app.add_option("-c,--config", config_file, "UTS config db location")->required()->check(CLI::ExistingFile);
	app.add_option("--new-password", new_password, "New password")->required();
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

	size_t account_num = uts.available_accounts().size();
	if (account_num != 1) {
		spdlog::error("There are {} active accounts, only one is supported.", account_num);
		return EXIT_FAILURE;
	}

	TradingAccount* account = uts.GetHandle(uts.available_accounts()[0]);
	try {
		account->LogOnSync();
	} catch (const FirstLoginPasswordNeedChangeError& e) {}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	account->UpdatePassword(new_password);
}

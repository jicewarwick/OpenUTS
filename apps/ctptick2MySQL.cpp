#include <chrono>
#include <thread>
#include <vector>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "ctpmarketdatarecorder.h"
#include "dbconfig.h"
#include "mariadbdatarecorder.h"
#include "trading_utils.h"

using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
	std::filesystem::path config_file;

	CLI::App app{"Dump CTP tick data into mariadb"};
	app.add_option("-c,--config", config_file, "UTS config db location")->required()->check(CLI::ExistingFile);
	CLI11_PARSE(app, argc, argv)

	UTSConfigDB db(config_file);
	std::vector<Ticker> tickers = db.GetSubscriptionTickers();

	std::vector<IPAddress> md_server = db.FartestCTPMDServers(2);
	MySQLConnectionInfo db_info = db.GetMySQLConnectionInfo();

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

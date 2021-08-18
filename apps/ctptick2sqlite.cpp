#include <chrono>
#include <ranges>
#include <vector>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "ctpmarketdatarecorder.h"
#include "dbconfig.h"
#include "sqlite3datarecorder.h"
#include "trading_utils.h"

using std::string, std::vector;
using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
	std::filesystem::path config_file;
	string db_name = "tick.sqlite3";

	CLI::App app{"Dump CTP tick data into sqlite3 database"};
	app.add_option("-c,--config", config_file, "UTS config db location")->required()->check(CLI::ExistingFile);
	app.add_option("-o,--output", db_name, "output sqlite3 file");
	CLI11_PARSE(app, argc, argv)

	UTSConfigDB db(config_file);
	vector<Ticker> tickers = db.GetSubscriptionTickers();
	vector<IPAddress> md_server = db.FartestCTPMDServers(2);

	auto end_time = GetMarketCloseTime() + 2min;

	auto dot_loc = db_name.find('.');
// TODO: C++20 fmt replace
#include <ctime>
#include <iomanip>
#include <sstream>
	auto end_time_t = std::chrono::system_clock::to_time_t(end_time);
	std::stringstream ss;
	ss << std::put_time(localtime(&end_time_t), "%Y%m%d");
	db_name = db_name.substr(0, dot_loc) + "_" + ss.str() + db_name.substr(dot_loc);
	// db_name = std::format("{}_{:%F}{}", db_name.substr(0, dot_loc), end_time, db_name.substr(dot_loc));

	SQLite3DataRecorder sink(db_name);
	CTPMarketDataRecorder market_data_recorder(md_server);
	market_data_recorder.setSink(&sink);
	market_data_recorder.LogIn();
	market_data_recorder.Subscribe(tickers);

	// TODO: C++20 fmt replace
	spdlog::info("Program ends at {}", ctime(&end_time_t));
	// spdlog::info(std::format("Program ends at {:%c}", end_time));
	std::this_thread::sleep_until(end_time);
	// std::this_thread::sleep_for(1min);
}

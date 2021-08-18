#include <filesystem>
#include <ranges>
#include <vector>

#include <uts/dbconfig.h>
#include <uts/utsexceptions.h>

using std::string, std::vector, std::map;

UTSConfigDB::UTSConfigDB(const std::filesystem::path& db_name) {
	sqlite3_config(SQLITE_CONFIG_SERIALIZED);
	int error_code = sqlite3_open(db_name.string().c_str(), &conn_);
	if (error_code != SQLITE_OK) {
		auto msg = sqlite3_errstr(error_code);
		sqlite3_close(conn_);
		throw DBFileError(msg);
	}
	const char* init_query =
		"CREATE TABLE IF NOT EXISTS subscribe_contracts(index INTEGER, ticker TEXT, end_date TEXT);"
		"CREATE TABLE IF NOT EXISTS ctp_md_latency(datetime DATETIME DEFAULT CURRENT_TIMESTAMP, address TEXT NOT NULL, "
		"latency INT NULL DEFAULT NULL);"
		"CREATE TABLE IF NOT EXISTS ctp_md_server(index INTEGER, name TEXT, address TEXT);"
		"CREATE TABLE IF NOT EXISTS ctp_trade_server(index INTEGER, broker TEXT, server_name TEXT, trade_server TEXT, "
		"broker_id TEXT, AppID TEXT, AuthCode TEXT);"
		"CREATE TABLE IF NOT EXISTS ctp_account(index INTEGER, account_name TEXT, broker_name TEXT, account_number "
		"TEXT, password TEXT, enable INTEGER);"
		"CREATE TABLE mysql_connection_info(addr TEXT, db_name TEXT, user_name TEXT, password TEXT);"
		"CREATE TABLE no_close_today_tickers(index INTEGER, ticker TEXT);";

	sqlite3_exec(conn_, init_query, nullptr, nullptr, nullptr);
}

UTSConfigDB::UTSConfigDB(UTSConfigDB&& rhs) noexcept {
	if (this != &rhs) {
		conn_ = rhs.conn_;
		rhs.conn_ = nullptr;
	}
}

UTSConfigDB::~UTSConfigDB() {
	if (conn_) { sqlite3_close(conn_); }
}

auto list_one_col_func = [](void* closure, int, char** argv, char**) -> int {
	auto dummy = reinterpret_cast<vector<string>*>(closure);
	dummy->emplace_back(argv[0]);
	return 0;
};

vector<IPAddress> UTSConfigDB::UnSpeedTestedCTPMDServers() const {
	vector<IPAddress> ret;
	const char* query = "SELECT address FROM ctp_md_server WHERE address NOT IN (SELECT address FROM ctp_md_latency);";
	sqlite3_exec(conn_, query, list_one_col_func, &ret, nullptr);
	return ret;
}

vector<IPAddress> UTSConfigDB::FartestCTPMDServers(size_t n) const {
	vector<IPAddress> ret;
	// TODO: C++20 fmt replace
	string query = "SELECT address FROM ctp_md_latency ORDER BY latency ASC LIMIT " + std::to_string(n);
	// string query = std::format("SELECT address FROM ctp_md_latency ORDER BY latency ASC LIMIT {};", n);
	sqlite3_exec(conn_, query.c_str(), list_one_col_func, &ret, nullptr);
	return ret;
}
void UTSConfigDB::AppendSpeedTestResult(const IPAddress& addr, std::chrono::milliseconds ms) {
	// TODO: C++20 fmt replace
	string query =
		"INSERT INTO ctp_md_latency(address, latency) VALUES(" + addr + ", " + std::to_string(ms.count()) + " );";
	// string query = std::format("INSERT INTO ctp_md_latency (address, latency) VALUES ({}, {});", addr, ms.count());
	sqlite3_exec(conn_, query.c_str(), nullptr, nullptr, nullptr);
}

auto list_one_col_set_func = [](void* closure, int, char** argv, char**) -> int {
	auto dummy = reinterpret_cast<std::set<string>*>(closure);
	dummy->insert(argv[0]);
	return 0;
};

vector<Ticker> UTSConfigDB::GetSubscriptionTickers() const {
	vector<Ticker> ret;
	const char* query = "SELECT ticker FROM subscribe_contracts;";
	sqlite3_exec(conn_, query, list_one_col_func, &ret, nullptr);
	return ret;
}

std::set<Ticker> UTSConfigDB::GetNoCloseTodayContracts() const {
	std::set<Ticker> ret;
	const char* query = "SELECT ticker FROM no_close_today_tickers;";
	sqlite3_exec(conn_, query, list_one_col_set_func, &ret, nullptr);
	return ret;
}

auto to_string = [](auto&& r) -> std::string {
	const auto data = &*r.begin();
	const auto size = static_cast<std::size_t>(std::ranges::distance(r));
	return std::string{data, size};
};

auto split(const std::string& str, char delimiter) -> std::vector<std::string> {
	const auto range = str | std::ranges::views::split(delimiter) | std::ranges::views::transform(to_string);
	return {std::ranges::begin(range), std::ranges::end(range)};
}

auto broker_info_map_func = [](void* closure, int, char** argv, char**) -> int {
	std::vector<std::string> ips = split(argv[1], ',');
	BrokerInfo info{
		.broker_name = argv[0],
		.API_type = APIType::CTP,
		.broker_id = argv[2],
		.trade_server_addr = ips,
		.app_id = argv[3],
		.auth_code = argv[4],
	};

	auto dummy = reinterpret_cast<map<BrokerName, BrokerInfo>*>(closure);
	dummy->insert({info.broker_name, info});
	return 0;
};

map<BrokerName, BrokerInfo> UTSConfigDB::GetBrokerInfo() const {
	map<BrokerName, BrokerInfo> ret;
	//								 0,			   1,		  2,	 3,		   4
	const char* query = "SELECT broker, trade_server, broker_id, AppID, AuthCode FROM ctp_trade_server;";
	sqlite3_exec(conn_, query, broker_info_map_func, &ret, nullptr);
	return ret;
}

auto account_info_map_func = [](void* closure, int, char** argv, char**) -> int {
	AccountInfo info{
		.account_name = argv[0],
		.broker_name = argv[1],
		.account_number = argv[2],
		.password = argv[3],
		.enable = static_cast<bool>(argv[4]),
	};

	auto dummy = reinterpret_cast<vector<AccountInfo>*>(closure);
	dummy->push_back(info);
	return 0;
};

vector<AccountInfo> UTSConfigDB::GetAccountInfo() const {
	vector<AccountInfo> ret;
	//								 	   0,			1,			    2,		  3,	  4
	const char* query = "SELECT account_name, broker_name, account_number, password, enable FROM ctp_account;";
	sqlite3_exec(conn_, query, account_info_map_func, &ret, nullptr);
	return ret;
}

auto mysql_info_func = [](void* closure, int, char** argv, char**) -> int {
	MySQLConnectionInfo info{
		.addr = argv[0],
		.db_name = argv[1],
		.user_name = argv[2],
		.password = argv[3],
	};

	auto dummy = reinterpret_cast<MySQLConnectionInfo*>(closure);
	*dummy = info;
	return 0;
};

MySQLConnectionInfo UTSConfigDB::GetMySQLConnectionInfo() const {
	MySQLConnectionInfo info;
	const char* query = "SELECT addr, db_name, user_name, password FROM mysql_connection_info;";
	sqlite3_exec(conn_, query, mysql_info_func, &info, nullptr);
	if (info.addr.empty()) { throw DBFileError("mysql_connection_info in config db is empty"); }
	return info;
}

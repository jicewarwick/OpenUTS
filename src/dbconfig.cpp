#include <uts/dbconfig.h>
#include <uts/utsexceptions.h>

using std::string, std::vector;

auto list_one_col_func = [](void* closure, int argc, char** argv, char**) -> int {
	auto dummp = reinterpret_cast<vector<string>*>(closure);
	dummp->reserve(argc);
	for (int i = 0; i < argc; ++i) { dummp->emplace_back(argv[i]); }
	return 0;
};

UTSConfigDB::UTSConfigDB(const std::filesystem::path& db_name) {
	sqlite3_config(SQLITE_CONFIG_SERIALIZED);
	int error_code = sqlite3_open(db_name.string().c_str(), &conn_);
	if (error_code != SQLITE_OK) {
		auto msg = sqlite3_errstr(error_code);
		sqlite3_close(conn_);
		throw DBFileError(msg);
	}
	const char* init_query =
		"CREATE TABLE IF NOT EXISTS broker_id(index INTEGER, broker TEXT, broker_id TEXT);"
		"CREATE TABLE IF NOT EXISTS cffex_contracts(index INTEGER, ticker TEXT, end_date TEXT);"
		"CREATE TABLE IF NOT EXISTS ctp_md_latency(datetime DATETIME DEFAULT CURRENT_TIMESTAMP, address TEXT NOT NULL, "
		"latency INT NULL DEFAULT NULL);"
		"CREATE TABLE IF NOT EXISTS ctp_md_server(index INTEGER, name TEXT, address TEXT);"
		"CREATE TABLE IF NOT EXISTS ctp_trade_server(index INTEGER, broker TEXT, server_name TEXT, trade_server "
		"TEXT); ";

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

vector<Ticker> UTSConfigDB::CurrentIFIOTickers() const {
	vector<Ticker> ret;
	const char* query = "SELECT ticker FROM cffex_contracts;";
	sqlite3_exec(conn_, query, list_one_col_func, &ret, nullptr);
	return ret;
}

#include "sqlite3datarecorder.h"

#include <stdexcept>

SQLite3DataRecorder::SQLite3DataRecorder(const std::filesystem::path& db_name) {
	int error_code = sqlite3_open(db_name.string().c_str(), &conn_);
	if (error_code != SQLITE_OK) {
		auto msg = sqlite3_errstr(error_code);
		sqlite3_close(conn_);
		throw std::runtime_error(msg);
	}
	const char* create_table_query =
		"CREATE TABLE IF NOT EXISTS tickdata ( \
		DateTime DATETIME NOT NULL, \
		ID VARCHAR(20) NOT NULL, \
		Open DECIMAL(6,3) NULL DEFAULT NULL, \
		High DECIMAL(6,3) NULL DEFAULT NULL, \
		Low DECIMAL(6,3) NULL DEFAULT NULL, \
		Latest DECIMAL(6,3) NULL DEFAULT NULL, \
		TurnOver BIGINT(20) NULL DEFAULT NULL, \
		Volume BIGINT(20) NULL DEFAULT NULL, \
		OpenInterest BIGINT(20) NULL DEFAULT NULL, \
		BidPrice1 DECIMAL(6,3) NULL DEFAULT NULL, \
		BidVolume1 INT(11) NULL DEFAULT NULL, \
		AskPrice1 DECIMAL(6,3) NULL DEFAULT NULL, \
		AskVolume1 INT(11) NULL DEFAULT NULL \
		)";
	sqlite3_exec(conn_, create_table_query, nullptr, nullptr, nullptr);

	const char* query =
		"INSERT INTO tickdata (DateTime, ID, Open, High, Low, Latest, TurnOver, Volume, OpenInterest, BidPrice1, "
		"BidVolume1, AskPrice1, AskVolume1) VALUES (:DateTime, :ID, :Open, :High, :Low, :Latest, :TurnOver, :Volume, "
		":OpenInterest, :BidPrice1, :BidVolume1, :AskPrice1, :AskVolume1);";
	sqlite3_prepare_v2(conn_, query, -1, &stmt_, nullptr);
}

SQLite3DataRecorder::~SQLite3DataRecorder() {
	if (conn_) {
		sqlite3_finalize(stmt_);
		sqlite3_close(conn_);
	}
}

void SQLite3DataRecorder::WriteDB(const MarketDepth& data) {
	sqlite3_reset(stmt_);
	sqlite3_clear_bindings(stmt_);
	sqlite3_bind_text(stmt_, sqlite3_bind_parameter_index(stmt_, ":DateTime"), data.update_time.c_str(), -1, nullptr);
	sqlite3_bind_text(stmt_, sqlite3_bind_parameter_index(stmt_, ":ID"), data.instrument_id.c_str(), -1, nullptr);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":Open"), data.ohlclvt.open);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":High"), data.ohlclvt.high);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":Low"), data.ohlclvt.low);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":Latest"), data.ohlclvt.last);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":TurnOver"), data.ohlclvt.turnover);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":Volume"), data.ohlclvt.volume);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":OpenInterest"), data.open_interest);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":BidPrice1"), data.bid[0].price);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":BidVolume1"), data.bid[0].volume);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":AskPrice1"), data.ask[0].price);
	sqlite3_bind_double(stmt_, sqlite3_bind_parameter_index(stmt_, ":AskVolume1"), data.ask[0].volume);

	sqlite3_step(stmt_);
}

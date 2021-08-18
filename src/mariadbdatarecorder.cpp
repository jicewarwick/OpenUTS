#include "mariadbdatarecorder.h"

#include <stdexcept>

#include <spdlog/spdlog.h>

MariadbDataRecorder::MariadbDataRecorder(const IPAddress& addr, const std::string& db_name, const UserName& user_name,
										 const Password& password) {
	// TODO: C++20 fmt replace
	std::string url = "jdbc:mariadb://" + addr + "/" + db_name;
	//	std::string url = std::format("jdbc:mariadb://{}/{}", addr, db_name);

	conn_ = sql::DriverManager::getConnection(url, user_name, password);

	sql::Statement* create_stmnt = conn_->createStatement();
	try {
		const char* create_table_query =
			"CREATE TABLE IF NOT EXISTS `tickdata` ( \
			`DateTime` DATETIME(6) NOT NULL, \
			`ID` VARCHAR(20) NOT NULL COLLATE 'utf8_general_ci', \
			`Open` DECIMAL(5,1) NULL DEFAULT NULL, \
			`High` DECIMAL(5,1) NULL DEFAULT NULL, \
			`Low` DECIMAL(5,1) NULL DEFAULT NULL, \
			`Latest` DECIMAL(5,1) NULL DEFAULT NULL, \
			`TurnOver` BIGINT(20) NULL DEFAULT NULL, \
			`Volume` BIGINT(20) NULL DEFAULT NULL, \
			`OpenInterest` BIGINT(20) NULL DEFAULT NULL, \
			`BidPrice1` DECIMAL(5,1) NULL DEFAULT NULL, \
			`BidVolume1` INT(11) NULL DEFAULT NULL, \
			`AskPrice1` DECIMAL(5,1) NULL DEFAULT NULL, \
			`AskVolume1` INT(11) NULL DEFAULT NULL, \
			PRIMARY KEY (`DateTime`, `ID`) USING BTREE, \
			INDEX `ID` (`ID`) USING BTREE \
			)";
		create_stmnt->executeUpdate(create_table_query);
	} catch (const sql::SQLException& e) {
		spdlog::error("Error creating tickdata table: {}", e.what());
		delete create_stmnt;
		throw e;
	}
	delete create_stmnt;

	const char* query =
		//                            1,  2,    3,    4,   5,      6,        7,      8,            9,        10,
		"INSERT INTO tickdata (DateTime, ID, Open, High, Low, Latest, TurnOver, Volume, OpenInterest, BidPrice1, "
		//       11,        12,         13
		"BidVolume1, AskPrice1, AskVolume1) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
	insert_stmnt_ = conn_->prepareStatement(query);
}

MariadbDataRecorder::MariadbDataRecorder(const MySQLConnectionInfo& info)
	: MariadbDataRecorder(info.addr, info.db_name, info.user_name, info.password) {}

MariadbDataRecorder::~MariadbDataRecorder() {
	delete insert_stmnt_;
	conn_->close();
}

void MariadbDataRecorder::WriteDB(const MarketDepth& data) {
	insert_stmnt_->setDateTime(1, data.update_time);
	insert_stmnt_->setString(2, data.instrument_id);
	insert_stmnt_->setDouble(3, data.ohlclvt.open);
	insert_stmnt_->setDouble(4, data.ohlclvt.high);
	insert_stmnt_->setDouble(5, data.ohlclvt.low);
	insert_stmnt_->setDouble(6, data.ohlclvt.last);
	insert_stmnt_->setDouble(7, data.ohlclvt.turnover);
	insert_stmnt_->setDouble(8, data.ohlclvt.volume);
	insert_stmnt_->setDouble(9, data.open_interest);
	insert_stmnt_->setDouble(10, data.bid[0].price);
	insert_stmnt_->setDouble(11, data.bid[0].volume);
	insert_stmnt_->setDouble(12, data.ask[0].price);
	insert_stmnt_->setDouble(13, data.ask[0].volume);

	try {
		insert_stmnt_->executeUpdate();
	} catch (sql::SQLException& e) { spdlog::error("Error adding contact to database: {}", e.what()); }
}

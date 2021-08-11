#pragma once

#include <mariadb/conncpp.hpp>
#include <uts/data_struct.h>
#include <uts/datarecorder.h>

/**
 * @brief Mariadb 数据记录器，将收到的 `MarketDepth` 信息写入 `CTPTick` 表中
 */
class MariadbDataRecorder : public DataRecorder {
public:
	/**
	 * @brief 构造函数.
	 * @param addr 数据库地址
	 * @param db_name 数据库名称
	 * @param user_name 数据库用户名
	 * @param password 数据库密码
	 * @exception std::runtime_error 无法打开数据库错误
	 */
	MariadbDataRecorder(const IPAddress& addr, const std::string& db_name, const UserName& user_name,
						const Password& password);
	MariadbDataRecorder(const MySQLConnectionInfo& info);

	virtual ~MariadbDataRecorder();

protected:
	void WriteDB(const MarketDepth&) override;

private:
	sql::Connection* conn_ = nullptr;
	sql::PreparedStatement* insert_stmnt_ = nullptr;
};

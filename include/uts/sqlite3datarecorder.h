#pragma once

#include <filesystem>
#include <sqlite3.h>

#include <uts/datarecorder.h>

/**
 * @brief SQLite3 数据记录器，将收到的 `MarketDepth` 信息写入 `.sqlite3` 文件
 */
class SQLite3DataRecorder : public DataRecorder {
public:
	/**
	 * @brief 构造函数.
	 * @param db_name `.sqlite3` 文件名称
	 * @exception std::runtime_error 无法打开数据库错误
	 */
	SQLite3DataRecorder(const std::filesystem::path& db_name);
	virtual ~SQLite3DataRecorder();

protected:
	void WriteDB(const MarketDepth&) override;

private:
	sqlite3* conn_ = nullptr;
	sqlite3_stmt* stmt_ = nullptr;
};

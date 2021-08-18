#pragma once

#include <filesystem>
#include <fstream>

#include <uts/datarecorder.h>

/**
 * @brief CSV 数据记录器，将收到的 `MarketDepth` 信息写入 `.csv` 文件
 */
class CSVDataRecorder : public DataRecorder {
public:
	/**
	 * @brief 构造函数.
	 * @param filename csv 文件名称
	 */
	CSVDataRecorder(const std::filesystem::path& filename);
	~CSVDataRecorder() override = default;

protected:
	void WriteDB(const MarketDepth& data) override;

private:
	std::ofstream out_;
};

#pragma once

#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>
#include <uts/utsexceptions.h>

/**
 * @brief 读取Json文件
 * @param path 文件路径
 * @exception `IOError`: 文件不存在
 * @exception `JsonFileError`: Json文件无法解析
 */
inline nlohmann::json ReadJsonFile(const std::filesystem::path& path) {
	if (!std::filesystem::exists(path)) { throw IOError(path); }
	std::ifstream file_stream(path);
	nlohmann::json info;
	try {
		info = nlohmann::json::parse(file_stream, nullptr, true, true);
	} catch (nlohmann::json::parse_error) { throw JsonFileError(path); }
	return info;
}

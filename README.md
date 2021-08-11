# OpenUnifiedTradingSystem(UTS)
统一的交易行情API(V0.1.0)

## 进度：
CTP:
- [x] 完成基本的交易和行情API封装
- [x] 行情收录
- [x] 统一的交易API部署
- [x] 其他功能
	- [x] CTP服务器信息获取和导入
	- [x] 手续费查询及导出
	- [x] 持仓成交接受导出(json)
	- [x] 看穿性测试

## 安装和把玩：
### 依赖:
- gcc 11.1 / MSVC 19.10
- cmake
- CTP library
- sqlite3
- nlohmann_json
- spdlog
- doxygen
- CLI11 (for command line executable)
- mariadbcpp (optional for mariadb)

CTP library需出现在：
- header: `${INCLUDE_PATH}/CTP/*.h`
- (windows) .lib file: `${PATH}/lib/`
- dll/so: `${PATH}/bin/`

## 文档
在 build 目录运行 `cmake --build . --target docs` 后在 `./docs` 下查看

## LICENSE
MIT License
# apps
include_directories(../include/)
include_directories(../include/uts)

find_package(CLI11 REQUIRED)

# CTPQueryer
add_executable(CTPQueyer ctpqueryer.cpp)
target_link_libraries(CTPQueyer PRIVATE UnifiedTradingSystem TradingUtils CLI11::CLI11 spdlog::spdlog)

# CTPCommissionRateQueyer
add_executable(CTPCommissionRateQueyer ctpcommissionratequeryer.cpp)
target_link_libraries(
	CTPCommissionRateQueyer PRIVATE UnifiedTradingSystem CLI11::CLI11 nlohmann_json::nlohmann_json spdlog::spdlog
)

# CTPTickSQLiteRecorder
add_executable(CTPTickSQLiteRecorder ctptick2sqlite.cpp)
target_link_libraries(
	CTPTickSQLiteRecorder
	PRIVATE SQLite3DataRecorder
			CTPMarketDataRecorder
			UnifiedTradingSystem
			TradingUtils
			CLI11::CLI11
			nlohmann_json::nlohmann_json
			spdlog::spdlog
)

# CTPTickMySQLRecorder
if(${MARIADBCPP_FOUND})
	add_executable(CTPTickMySQLRecorder ctptick2MySQL.cpp)
	target_link_libraries(
		CTPTickMySQLRecorder
		PRIVATE MariadbDataRecorder
				CTPMarketDataRecorder
				UnifiedTradingSystem
				TradingUtils
				CLI11::CLI11
				nlohmann_json::nlohmann_json
				spdlog::spdlog
	)
endif()

# SeeThroughTesting
add_executable(SeeThroughTesting seethroughtesting.cpp)
target_link_libraries(
	SeeThroughTesting PRIVATE UnifiedTradingSystem CTPUtils TradingUtils CLI11::CLI11 nlohmann_json::nlohmann_json
							  spdlog::spdlog
)

if(MSVC)
	target_compile_definitions(CTPQueyer PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_definitions(CTPTickSQLiteRecorder PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_definitions(CTPTickMySQLRecorder PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# installation
install(TARGETS CTPQueyer CTPCommissionRateQueyer CTPTickSQLiteRecorder SeeThroughTesting EXPORT ${PROJECT_NAME}Targets)
if(${MARIADBCPP_FOUND})
	install(TARGETS CTPTickMySQLRecorder EXPORT ${PROJECT_NAME}Targets)
endif()

install(FILES parse_broker_file.py DESTINATION bin)

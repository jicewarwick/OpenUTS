# src
include_directories(../include/)
include_directories(../include/uts)

# config
add_library(DBConfig dbconfig.cpp)
target_link_libraries(DBConfig PUBLIC SQLite::SQLite3)

# utils
add_library(TradingUtils trading_utils.cpp)
add_library(CTPUtils ctp_utils.cpp)
target_link_libraries(
	CTPUtils
	INTERFACE CTP::CTPMarketDataAPI
	PRIVATE spdlog::spdlog
)

# base interface
add_library(RateThrottler INTERFACE)
target_sources(
	RateThrottler INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/uts/ratethrottler.h>
							$<INSTALL_INTERFACE:include/uts/ratethrottler.h>
)
add_library(MarketData INTERFACE)
target_sources(
	MarketData INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/uts/market_data.h>
						 $<INSTALL_INTERFACE:include/uts/market_data.h>
)

# CTPMarketData
add_library(CTPMarketData ctpmarketdata.cpp)
target_link_libraries(
	CTPMarketData
	INTERFACE MarketData CTP::CTPMarketDataAPI
	PRIVATE CTPUtils spdlog::spdlog
)

# TradingAccount
add_library(TradingAccount tradingaccount.cpp)
target_link_libraries(TradingAccount PUBLIC nlohmann_json::nlohmann_json)
# CTPTradingAccount
add_library(CTPAccount ctptradingaccount.cpp)
target_link_libraries(
	CTPAccount
	PUBLIC TradingAccount
	INTERFACE RateThrottler CTP::CTPTraderAPI
	PRIVATE CTPUtils spdlog::spdlog
)

# UnifiedTradingSystem
add_library(UnifiedTradingSystem unifiedtradingsystem.cpp)
target_include_directories(UnifiedTradingSystem PRIVATE ${CMAKE_BINARY_DIR}/include/uts)
target_link_libraries(
	UnifiedTradingSystem
	PUBLIC CTPAccount CTPMarketData DBConfig
	PRIVATE TradingUtils spdlog::spdlog
)

# DataRecorder
add_library(DataRecorder datarecorder.cpp)
target_link_libraries(DataRecorder PRIVATE spdlog::spdlog)
# CSVDataRecorder
add_library(CSVDataRecorder csvdatarecorder.cpp)
target_link_libraries(
	CSVDataRecorder
	PUBLIC DataRecorder
	PRIVATE spdlog::spdlog
)
# SQLite3DataRecorder
add_library(SQLite3DataRecorder sqlite3datarecorder.cpp)
target_link_libraries(
	SQLite3DataRecorder
	PUBLIC DataRecorder SQLite::SQLite3
	PRIVATE spdlog::spdlog
)

# CTPMarketDataRecorder
add_library(CTPMarketDataRecorder ctpmarketdatarecorder.cpp)
target_link_libraries(
	CTPMarketDataRecorder
	PUBLIC CTPMarketData
	INTERFACE DataRecorder
	PRIVATE spdlog::spdlog
)

# MariadbDataRecorder
if(${MARIADBCPP_FOUND})
	add_library(MariadbDataRecorder mariadbdatarecorder.cpp)
	target_link_libraries(
		MariadbDataRecorder
		PUBLIC DataRecorder mariadb::mariadbcpp
		PRIVATE spdlog::spdlog
	)
endif()

if(MSVC)
	target_compile_definitions(CTPUtils PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
	target_compile_definitions(CTPAccount PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# installation
install(
	TARGETS RateThrottler
			DBConfig
			CTPUtils
			TradingUtils
			MarketData
			CTPMarketData
			TradingAccount
			CTPAccount
			UnifiedTradingSystem
			DataRecorder
			CSVDataRecorder
			SQLite3DataRecorder
	EXPORT ${PROJECT_NAME}Targets
)
if(${MARIADBCPP_FOUND})
	install(TARGETS MariadbDataRecorder EXPORT ${PROJECT_NAME}Targets)
endif()

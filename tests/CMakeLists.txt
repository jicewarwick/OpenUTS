# tests
include_directories(../include/)
include_directories(../include/uts)

include(CTest)
find_package(GTest REQUIRED)

add_executable(UtilsTest utils_test.cpp)
target_link_libraries(UtilsTest PRIVATE GTest::GTest DBConfig nlohmann_json::nlohmann_json)
gtest_discover_tests(UtilsTest)

add_executable(CTPMarketDataTest ctp_market_data_test.cpp)
target_link_libraries(CTPMarketDataTest PRIVATE CTPMarketData GTest::GTest spdlog::spdlog nlohmann_json::nlohmann_json)
gtest_discover_tests(CTPMarketDataTest)

add_executable(CTPAccountTest ctp_account_test.cpp)
target_link_libraries(CTPAccountTest PRIVATE CTPAccount GTest::GTest spdlog::spdlog)
gtest_discover_tests(CTPAccountTest)

add_executable(TradingSystemTest trading_system_test.cpp)
target_link_libraries(TradingSystemTest PRIVATE UnifiedTradingSystem GTest::GTest)
gtest_discover_tests(TradingSystemTest)

add_executable(DataRecorderTest data_recorder_test.cpp)
target_link_libraries(DataRecorderTest PRIVATE SQLite3DataRecorder CSVDataRecorder MariadbDataRecorder GTest::GTest)
gtest_discover_tests(DataRecorderTest)
file(
	COPY test_files
	DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
	FILES_MATCHING
	PATTERN "*.json,*.sqlite3"
)

if(INSTALL_TESTING)
	install(
		DIRECTORY test_files/
		DESTINATION ${CMAKE_INSTALL_PREFIX}/testing
		FILES_MATCHING
		PATTERN "*.json,*.sqlite3"
	)

	install(
		TARGETS JsonTest CTPMarketDataTest CTPAccountTest TradingSystemTest DataRecorderTest
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION bin
	)
endif(INSTALL_TESTING)

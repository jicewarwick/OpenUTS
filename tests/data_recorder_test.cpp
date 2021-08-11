#include <gtest/gtest.h>

#include "csvdatarecorder.h"
#include "data_struct.h"
#include "mariadbdatarecorder.h"
#include "sqlite3datarecorder.h"

MarketDepth md{"IC0000", "1111-11-11 11:11:11.111", {1, 1, 1, 1, 1, 1, 1}, 1, 1, 1, 1, 1, 1, {}, {}};

TEST(DataRecorderTest, CSVDateRecorderTest) {
	CSVDataRecorder recorder("test.csv");
	recorder.DataSink(md);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	recorder.DataSink(md);
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(DataRecorderTest, SQLiteDateRecorderTest) {
	SQLite3DataRecorder recorder("test.sqlite3");
	recorder.DataSink(md);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	recorder.DataSink(md);
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(DataRecorderTest, MariadbDateRecorderTest) {
	MariadbDataRecorder recorder("127.0.0.1", "asharedata", "ce", "123");
	recorder.DataSink(md);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	recorder.DataSink(md);
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

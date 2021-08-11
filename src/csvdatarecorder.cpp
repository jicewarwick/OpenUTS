#include "csvdatarecorder.h"

#include <spdlog/spdlog.h>

CSVDataRecorder::CSVDataRecorder(const std::filesystem::path& filename) {
	bool existed = std::filesystem::exists(filename);
	out_ = std::ofstream(filename);
	if (!existed) {
		out_ << "DateTime, ID, Open, High, Low, Latest, TurnOver, Volume, OpenInterest, "
				"BidPrice1, BidVolume1, AskPrice1, AskVolume1\n";
	}
}

void CSVDataRecorder::WriteDB(const MarketDepth& data) {
	out_ << data.update_time << ", ";
	out_ << data.instrument_id << ", " << data.ohlclvt.open << ", " << data.ohlclvt.high << ", " << data.ohlclvt.low
		 << ", " << data.ohlclvt.last << ", ";
	out_ << data.ohlclvt.turnover << ", " << data.ohlclvt.volume << ", ";
	out_ << data.open_interest << ", ";
	out_ << data.bid[0].price << ", " << data.bid[0].volume << ", " << data.ask[0].price << ", " << data.ask[0].volume
		 << "\n";
	spdlog::trace("market data processed.");
}

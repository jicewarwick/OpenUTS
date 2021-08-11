#include "ctpmarketdatarecorder.h"

#include <spdlog/spdlog.h>

void CTPMarketDataRecorder::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) {
	spdlog::trace("new market data received.");
	if (data_recorder_) {
		data_recorder_->DataSink(CTPMarketData2MarketDepth(pDepthMarketData));
	} else {
		spdlog::warn("No data recorder is specified. Data ignored!!!");
	}
}

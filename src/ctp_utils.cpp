#include "ctp_utils.h"

#include <codecvt>
#include <locale>
#include <random>
#include <string>

#include <spdlog/spdlog.h>

#include "utsexceptions.h"

using std::string, std::string_view;
namespace fs = std::filesystem;

string RandomFlowFolderName(unsigned int length) {
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> dist(0, 25);
	string s;
	s.reserve(length);
	for (unsigned int i = 0; i < length; ++i) { s.push_back(static_cast<char>('A' + dist(mt))); }
	return s;
}

fs::path CreateTempFlowFolder(string_view suffix) {
	fs::path cache_path_ = fs::temp_directory_path() /= RandomFlowFolderName(8) += suffix;
	if (!fs::exists(cache_path_)) {
		if (!fs::create_directory(cache_path_)) { throw(FlowFolderCreationError(cache_path_.string())); }
	}
	return cache_path_;
}

void DeleteTempFlowFolder(const fs::path& flow_path) noexcept {
	try {
		fs::remove_all(flow_path.native());
	} catch (fs::filesystem_error) {
		spdlog::warn("fail to remove temp folder: {}, please remove it manually.", flow_path.string());
		return;
	}
	spdlog::trace("removed temp folder: {}", flow_path.string());
}

class chs_codecvt : public std::codecvt_byname<wchar_t, char, std::mbstate_t> {
public:
#ifdef WIN32
	chs_codecvt() : codecvt_byname("chs") {}
#else
	chs_codecvt() : codecvt_byname("zh_CN.GB2312") {}
#endif
};

string GB2312ToUTF8(const string& gb2312) {
	std::wstring_convert<chs_codecvt> gb_converter;
	std::wstring wstr = gb_converter.from_bytes(gb2312);

	std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_converter;
	return utf8_converter.to_bytes(wstr);
}

void ErrorResponse(CThostFtdcRspInfoField* pRspInfo) {
	if (pRspInfo) {
		spdlog::error("CTPS: Error code: {}, Error Message: {}", pRspInfo->ErrorID, GB2312ToUTF8(pRspInfo->ErrorMsg));
	}
}

void RequestSendingConfirm(int code, string_view task) {
	switch (code) {
		case 0: spdlog::trace("CTP: {} request sent.", task); break;
		case -1: spdlog::error("CTP: {}: network error.", task); break;
		case -2: spdlog::error("CTP: {}: unprocessed requests excedded quota.", task); break;
		case -3: spdlog::error("CTP: {}: requests per second excedded quota.", task); break;
	}
}

InstrumentInfo TranslateInstrumentInfo(const CThostFtdcInstrumentField* const pInstrument) {
	InstrumentInfo info{
		.is_trading = (pInstrument->IsTrading == '1'),
		.instrument_id = pInstrument->InstrumentID,
		.instrument_name = GB2312ToUTF8(pInstrument->InstrumentName),
		.exchange = kExchangeTranslator.at(pInstrument->ExchangeID),
		.product_id = pInstrument->ProductID,
		.max_market_order_volume = pInstrument->MaxMarketOrderVolume,
		.min_market_order_volume = pInstrument->MinMarketOrderVolume,
		.max_limit_order_volume = pInstrument->MaxLimitOrderVolume,
		.min_limit_order_volume = pInstrument->MinLimitOrderVolume,
		.volume_multiplier = static_cast<double>(pInstrument->VolumeMultiple),
		.price_ticker = pInstrument->PriceTick,
		.start_deliver_date = pInstrument->StartDelivDate,
		.expire_date = pInstrument->ExpireDate,
		.use_max_margin_side_algorithm = (pInstrument->MaxMarginSideAlgorithm == THOST_FTDC_MMSA_YES),
	};

	if (pInstrument->ProductClass == THOST_FTDC_PC_Futures) {
		// future
		info.instrument_type = InstrumentType::Future;
		info.long_margin_ratio = pInstrument->LongMarginRatio;
		info.short_margin_ratio = pInstrument->ShortMarginRatio;
	} else {
		// option
		info.instrument_type = InstrumentType::Option;
		info.option_type = (pInstrument->OptionsType == THOST_FTDC_CP_CallOptions) ? OptionType::Call : OptionType::Put;
		info.underlying_instrument_id = pInstrument->UnderlyingInstrID;
		info.strike_price = pInstrument->StrikePrice;
		info.underlying_multiple = pInstrument->UnderlyingMultiple;
	}
	return info;
}

TradingRecord TradeField2TradingRecord(CThostFtdcTradeField* pTrade) {
	TradingRecord rec{
		.order_ref = atol(pTrade->OrderRef),
		.exchange = kExchangeTranslator.at(pTrade->ExchangeID),
		.instrument_id = pTrade->InstrumentID,
		.open_close = kOpenCloseTranslator.at(pTrade->OffsetFlag),
		.direction = (pTrade->Direction == THOST_FTDC_D_Buy) ? Direction::Long : Direction::Short,
		.hedge_flag = kHedgeFlagTranslator.at(pTrade->HedgeFlag),
		.price = pTrade->Price,
		.volume = pTrade->Volume,
		.time = pTrade->TradeTime,
	};
	return rec;
}

OrderRecord OrderField2OrderRecord(CThostFtdcOrderField* pOrder) {
	OrderRecord rec{
		.front_id = pOrder->FrontID,
		.session_id = pOrder->SessionID,
		.order_ref = atol(pOrder->OrderRef),
		.exchange = kExchangeTranslator.at(pOrder->ExchangeID),
		.instrument_id = pOrder->InstrumentID,
		.open_close = kOpenCloseTranslator.at(pOrder->CombOffsetFlag[0]),
		.direction = (pOrder->Direction == THOST_FTDC_D_Buy) ? Direction::Long : Direction::Short,
		.hedge_flag = kHedgeFlagTranslator.at(pOrder->CombHedgeFlag[0]),
		.total_volume = pOrder->VolumeTotalOriginal,
		.traded_volume = pOrder->VolumeTraded,
		.remained_volume = pOrder->VolumeTotal,
		.order_price_type = kOrderPriceTypeTranslator.at(pOrder->OrderPriceType),
		.limit_price = pOrder->LimitPrice,
		.time_condition = kTimeConditionTranslator.at(pOrder->TimeCondition),
		.reference_price = pOrder->StopPrice,
		.order_status = kOrderStatusTranslator.at(pOrder->OrderStatus),
		.time = pOrder->InsertTime,
	};
	return rec;
}

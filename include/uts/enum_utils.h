#pragma once

#include <nlohmann/json.hpp>
#include <uts/data_struct.h>

using nlohmann::json;

NLOHMANN_JSON_SERIALIZE_ENUM(Exchange, {
										   {Exchange::NA, "NA"},
										   {Exchange::SHF, "SHF"},
										   {Exchange::DCE, "DCE"},
										   {Exchange::CZC, "CZC"},
										   {Exchange::CFE, "CFE"},
										   {Exchange::INE, "INE"},
										   {Exchange::SH, "SH"},
										   {Exchange::SZ, "SZ"},
										   {Exchange::SGE, "SGE"},
										   {Exchange::OC, "OC"},
										   {Exchange::HK, "HK"},
									   })

NLOHMANN_JSON_SERIALIZE_ENUM(InstrumentType, {
												 {InstrumentType::Stock, "stock"},
												 {InstrumentType::Future, "future"},
												 {InstrumentType::Option, "option"},
												 {InstrumentType::FutureOption, "future_option"},
												 {InstrumentType::Bond, "bond"},
												 {InstrumentType::FX, "forex"},
												 {InstrumentType::Index, "index"},
												 {InstrumentType::ETF, "etf"},
												 {InstrumentType::MultiLeg, "multileg"},
												 {InstrumentType::Synthetic, "synthetic"},
												 {InstrumentType::LOF, "lof"},
											 })

NLOHMANN_JSON_SERIALIZE_ENUM(Direction, {
											{Direction::Long, "long"},
											{Direction::Short, "short"},
										})

NLOHMANN_JSON_SERIALIZE_ENUM(OptionType, {
											 {OptionType::NotApplicable, "NA"},
											 {OptionType::Call, "call"},
											 {OptionType::Put, "put"},
										 })

NLOHMANN_JSON_SERIALIZE_ENUM(APIType, {
										  {APIType::CTP, "CTP"},
									  })

NLOHMANN_JSON_SERIALIZE_ENUM(OpenCloseType, {
												{OpenCloseType::Auto, "auto"},
												{OpenCloseType::Open, "open"},
												{OpenCloseType::Close, "close"},
												{OpenCloseType::CloseToday, "close_today"},
												{OpenCloseType::CloseYesterday, "close_yesterday"},
											})

NLOHMANN_JSON_SERIALIZE_ENUM(HedgeFlagType, {
												{HedgeFlagType::Hedge, "hedge"},
												{HedgeFlagType::Covered, "cover"},
												{HedgeFlagType::Arbitrage, "arbitrage"},
												{HedgeFlagType::MarketMaker, "market_maker"},
												{HedgeFlagType::Speculation, "speculation"},
											})
NLOHMANN_JSON_SERIALIZE_ENUM(OrderPriceType, {
												 {OrderPriceType::AnyPrice, "any_price"},
												 {OrderPriceType::LimitPrice, "limit_price"},
												 {OrderPriceType::BestPrice, "best_price"},
												 {OrderPriceType::LastPrice, "last_price"},
												 {OrderPriceType::BidPrice, "bid_price"},
												 {OrderPriceType::AskPrice, "ask_price"},
												 {OrderPriceType::FiveLevelPrice, "five_level_price"},
											 })

NLOHMANN_JSON_SERIALIZE_ENUM(TimeInForce, {
											  {TimeInForce::GFD, "GFD"},
											  {TimeInForce::FOK, "FOK"},
											  {TimeInForce::FAK, "FAK"},
										  })

NLOHMANN_JSON_SERIALIZE_ENUM(TimeCondition, {
												{TimeCondition::GFD, "GFD"},
												{TimeCondition::IOC, "IOC"},
											})

NLOHMANN_JSON_SERIALIZE_ENUM(VolumeCondition, {
												  {VolumeCondition::AllVolume, "all_volume"},
												  {VolumeCondition::AnyVolume, "any_volume"},
											  })

NLOHMANN_JSON_SERIALIZE_ENUM(
	OrderContingentCondition,
	{
		{OrderContingentCondition::Immediately, "immediately"},
		{OrderContingentCondition::Touch, "touch"},
		{OrderContingentCondition::TouchProfit, "touch_profit"},
		{OrderContingentCondition::ParkedOrder, "parked_order"},
		{OrderContingentCondition::LastPriceGreaterThanReferencePrice, "last_price>reference_price"},
		{OrderContingentCondition::LastPriceGreaterEqualReferencePrice, "last_price>=reference_price"},
		{OrderContingentCondition::LastPriceLesserThanReferencePrice, "last_price<reference_price"},
		{OrderContingentCondition::LastPriceLesserEqualReferencePrice, "last_price<=reference_price"},
		{OrderContingentCondition::AskPriceGreaterThanReferencePrice, "ask_price>reference_price"},
		{OrderContingentCondition::AskPriceGreaterEqualReferencePrice, "ask_price>=reference_price"},
		{OrderContingentCondition::AskPriceLesserThanReferencePrice, "ask_price<reference_price"},
		{OrderContingentCondition::AskPriceLesserEqualReferencePrice, "ask_price<=reference_price"},
		{OrderContingentCondition::BidPriceGreaterThanReferencePrice, "bid_price>reference_price"},
		{OrderContingentCondition::BidPriceGreaterEqualReferencePrice, "bid_price>=reference_price"},
		{OrderContingentCondition::BidPriceLesserThanReferencePrice, "bid_price<reference_price"},
		{OrderContingentCondition::BidPriceLesserEqualReferencePrice, "bid_price<=reference_price"},
	})

NLOHMANN_JSON_SERIALIZE_ENUM(OrderStatus, {
											  {OrderStatus::AllTraded, "all traded"},
											  {OrderStatus::PartTradedQueueing, "part traded still queueing"},
											  {OrderStatus::PartTradedNotQueueing, "part traded not queueing"},
											  {OrderStatus::NoTradeQueueing, "no trade still queueing"},
											  {OrderStatus::NoTradeNotQueueing, "no trade not queueing"},
											  {OrderStatus::RejectedByServer, "rejected by broker server"},
											  {OrderStatus::RejectedByExchange, "rejected by exchange"},
											  {OrderStatus::Canceled, "canceled"},
											  {OrderStatus::Unknown, "unknown"},
											  {OrderStatus::NotTouched, "not touched"},
											  {OrderStatus::Touched, "touched"},
										  });

// json type conversion
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AccountInfo, account_name, broker_name, account_number, password, enable)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BrokerInfo, broker_name, API_type, broker_id, user_product_info, trade_server_addr,
								   auth_code, app_id)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MySQLConnectionInfo, addr, db_name, user_name, password)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InstrumentInfo, instrument_type, instrument_id, instrument_name, exchange,
								   product_id, volume_multiplier, price_ticker, long_margin_ratio, short_margin_ratio,
								   use_max_margin_side_algorithm, underlying_instrument_id, strike_price,
								   underlying_multiple, option_type)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InstrumentIndex, instrument_id, direction, hedge_flag)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InstrumentCommissionRate, instrument_id, open_ratio_by_money, open_ratio_by_volume,
								   close_ratio_by_money, close_ratio_by_volume, close_today_ratio_by_money,
								   close_today_ratio_by_volume)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CapitalInfo, balance, margin_used, available, commission, withdraw_allowance)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HoldingRecord, exchange, instrument_id, hedge_flag, direction, total_quantity,
								   today_quantity, pre_quantity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TradingRecord, order_ref, exchange, instrument_id, hedge_flag, open_close, direction,
								   price, volume, time)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OrderRecord, front_id, session_id, order_ref, exchange, instrument_id, hedge_flag,
								   open_close, direction, limit_price, total_volume, traded_volume, remained_volume,
								   order_price_type, reference_price, time_condition, contingent_condition,
								   reference_price, order_status, time)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Order, account_name, broker_name, exchange, instrument_id, open_close, hedge_flag,
								   direction, volume, order_price_type, limit_price, tick_offset, level_offset,
								   time_in_force, contingent_condition, reference_price)

inline void to_json(json& j, const MarketDepth& m) {
	j = json{
		{"instrument_id", m.instrument_id},
		{"open", m.ohlclvt.open},
		{"high", m.ohlclvt.high},
		{"low", m.ohlclvt.low},
		{"close", m.ohlclvt.close},
		{"last", m.ohlclvt.last},
		{"volume", m.ohlclvt.volume},
		{"turnover", m.ohlclvt.turnover},
		{"settle", m.settle},
		{"delta", m.delta},
		{"upper_limit", m.upper_limit},
		{"lower_limit", m.lower_limit},
	};
}

#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

#include <CTP/ThostFtdcUserApiStruct.h>
#include <uts/data_struct.h>

/// 返回长度为`length`的随机文件夹名称
std::string RandomFlowFolderName(unsigned int length);
/**
 * @brief 在临时文件夹创建后缀为 `suffix` 的文件夹
 * @exception FlowFolderCreationError 无法建立临时文件夹失败
 */
std::filesystem::path CreateTempFlowFolder(std::string_view suffix);
/// 删除 `flow_path` 文件夹
void DeleteTempFlowFolder(const std::filesystem::path& flow_path) noexcept;

/// 从合约代码获取期货品种
inline std::string GetProductID(const Ticker& ticker) {
	return std::isalpha(ticker[1]) ? ticker.substr(0, 2) : ticker.substr(0, 1);
}

/// 将无效数据替换成 -1
inline double SanitizeData(double data) { return data < 1e308 ? data : -1.0; }

/// 将GB2312编码的string转成UTF-8的
std::string GB2312ToUTF8(const std::string& gb2312);

/// 验证请求是否已成功发出
void RequestSendingConfirm(int code, std::string_view task);

/// 错误信息提示
void ErrorResponse(CThostFtdcRspInfoField* pRspInfo);

/// 将CTP提供的合约信息转换为InstrumentInfo
InstrumentInfo TranslateInstrumentInfo(const CThostFtdcInstrumentField* const pInstrument);

/// 将CTP提供的成交信息转换为TradeRecord
TradingRecord TradeField2TradingRecord(CThostFtdcTradeField* pTrade);

/// 将CTP提供的委托信息转换为OrderRecord
OrderRecord OrderField2OrderRecord(CThostFtdcOrderField* pOrder);

/// 系统和三方Enum转换工具。创建后可通过 `at` 查找到对应的值
template <class T1, class T2>
requires(!std::same_as<T1, T2>) class EnumTranslator {
public:
	EnumTranslator(const std::map<T1, T2>& m) : map1_(m) {
		for (const auto& it : m) { map2_.insert({it.second, it.first}); }
	}
	~EnumTranslator() = default;

	T2 at(T1 t1) const { return map1_.at(t1); }
	T1 at(T2 t2) const { return map2_.at(t2); }

private:
	const std::map<T1, T2> map1_;
	std::map<T2, T1> map2_;
};
const EnumTranslator kExchangeTranslator{std::map<std::string, Exchange>{
	{"", Exchange::NA},
	{"SHFE", Exchange::SHF},
	{"DCE", Exchange::DCE},
	{"CZCE", Exchange::CZC},
	{"CFFEX", Exchange::CFE},
	{"INE", Exchange::INE},
}};

const EnumTranslator kContingentConditionTranslator{
	std::map<OrderContingentCondition, TThostFtdcContingentConditionType>{
		{OrderContingentCondition::Immediately, THOST_FTDC_CC_Immediately},
		{OrderContingentCondition::Touch, THOST_FTDC_CC_Touch},
		{OrderContingentCondition::TouchProfit, THOST_FTDC_CC_TouchProfit},
		{OrderContingentCondition::ParkedOrder, THOST_FTDC_CC_ParkedOrder},
		{OrderContingentCondition::LastPriceGreaterThanReferencePrice, THOST_FTDC_CC_LastPriceGreaterThanStopPrice},
		{OrderContingentCondition::LastPriceGreaterEqualReferencePrice, THOST_FTDC_CC_LastPriceGreaterEqualStopPrice},
		{OrderContingentCondition::LastPriceLesserThanReferencePrice, THOST_FTDC_CC_LastPriceLesserThanStopPrice},
		{OrderContingentCondition::LastPriceLesserEqualReferencePrice, THOST_FTDC_CC_LastPriceLesserEqualStopPrice},
		{OrderContingentCondition::AskPriceGreaterThanReferencePrice, THOST_FTDC_CC_AskPriceGreaterThanStopPrice},
		{OrderContingentCondition::AskPriceGreaterEqualReferencePrice, THOST_FTDC_CC_AskPriceGreaterEqualStopPrice},
		{OrderContingentCondition::AskPriceLesserThanReferencePrice, THOST_FTDC_CC_AskPriceLesserThanStopPrice},
		{OrderContingentCondition::AskPriceLesserEqualReferencePrice, THOST_FTDC_CC_AskPriceLesserEqualStopPrice},
		{OrderContingentCondition::BidPriceGreaterThanReferencePrice, THOST_FTDC_CC_BidPriceGreaterThanStopPrice},
		{OrderContingentCondition::BidPriceGreaterEqualReferencePrice, THOST_FTDC_CC_BidPriceGreaterEqualStopPrice},
		{OrderContingentCondition::BidPriceLesserThanReferencePrice, THOST_FTDC_CC_BidPriceLesserThanStopPrice},
		{OrderContingentCondition::BidPriceLesserEqualReferencePrice, THOST_FTDC_CC_BidPriceLesserEqualStopPrice},
	}};
const EnumTranslator kTimeConditionTranslator{std::map<TimeCondition, TThostFtdcTimeConditionType>{
	{TimeCondition::IOC, THOST_FTDC_TC_IOC},
	{TimeCondition::GFD, THOST_FTDC_TC_GFD},
}};
const EnumTranslator kOpenCloseTranslator{std::map<OpenCloseType, TThostFtdcOffsetFlagEnType>{
	{OpenCloseType::Open, THOST_FTDC_OFEN_Open},
	{OpenCloseType::Close, THOST_FTDC_OFEN_Close},
	{OpenCloseType::ForceClose, THOST_FTDC_OFEN_ForceClose},
	{OpenCloseType::CloseToday, THOST_FTDC_OFEN_CloseToday},
	{OpenCloseType::CloseYesterday, THOST_FTDC_OFEN_CloseYesterday},
	{OpenCloseType::ForceClose, THOST_FTDC_OFEN_ForceOff},
	{OpenCloseType::LocalForceOff, THOST_FTDC_OFEN_LocalForceClose},
}};
const EnumTranslator kHedgeFlagTranslator{std::map<HedgeFlagType, TThostFtdcHedgeFlagEnType>{
	{HedgeFlagType::Speculation, THOST_FTDC_HFEN_Speculation},
	{HedgeFlagType::Arbitrage, THOST_FTDC_HFEN_Arbitrage},
	{HedgeFlagType::Hedge, THOST_FTDC_HFEN_Hedge},
}};
const EnumTranslator kOrderPriceTypeTranslator{std::map<OrderPriceType, TThostFtdcOrderPriceTypeType>{
	{OrderPriceType::AnyPrice, THOST_FTDC_OPT_AnyPrice},
	{OrderPriceType::LimitPrice, THOST_FTDC_OPT_LimitPrice},
	{OrderPriceType::BestPrice, THOST_FTDC_OPT_BestPrice},
	{OrderPriceType::LastPrice, THOST_FTDC_OPT_LastPrice},
	{OrderPriceType::FiveLevelPrice, THOST_FTDC_OPT_FiveLevelPrice},
}};
const EnumTranslator kOrderStatusTranslator{std::map<OrderStatus, TThostFtdcOrderStatusType>{
	{OrderStatus::AllTraded, THOST_FTDC_OST_AllTraded},
	{OrderStatus::PartTradedQueueing, THOST_FTDC_OST_PartTradedQueueing},
	{OrderStatus::PartTradedNotQueueing, THOST_FTDC_OST_PartTradedNotQueueing},
	{OrderStatus::NoTradeQueueing, THOST_FTDC_OST_NoTradeQueueing},
	{OrderStatus::NoTradeNotQueueing, THOST_FTDC_OST_NoTradeNotQueueing},
	{OrderStatus::Canceled, THOST_FTDC_OST_Canceled},
	{OrderStatus::Unknown, THOST_FTDC_OST_Unknown},
	{OrderStatus::NotTouched, THOST_FTDC_OST_NotTouched},
	{OrderStatus::Touched, THOST_FTDC_OST_Touched},
}};

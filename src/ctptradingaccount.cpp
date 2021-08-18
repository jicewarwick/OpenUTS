#include "ctptradingaccount.h"

#include <algorithm>
#include <charconv>
#include <chrono>

#include <spdlog/spdlog.h>

#include "ctp_utils.h"
#include "enum_utils.h"
#include "trading_utils.h"
#include "utsexceptions.h"

using std::scoped_lock, std::unique_lock, std::chrono::seconds, std::mutex, std::string, std::vector, std::map;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

/**
 * @brief CTPTradingAccount 构造函数, 需提供账户和经纪商信息
 *
 * 构造期间需创建临时文件夹. 创建失败则抛出 `FlowFolderCreationError`.
 * @param account_info 交易账户信息
 * @param ctp_broker_info 经纪商信息
 * @exception FlowFolderCreationError 无法创建临时文件夹
 */
CTPTradingAccount::CTPTradingAccount(const AccountInfo& account_info, const BrokerInfo& ctp_broker_info)
	: TradingAccount(account_info), trade_server_addr_(ctp_broker_info.trade_server_addr),
	  broker_id_(ctp_broker_info.broker_id), user_product_info_(ctp_broker_info.user_product_info),
	  auth_code_(ctp_broker_info.auth_code), app_id_(ctp_broker_info.app_id) {
	connection_status_ = ConnectionStatus::Initializing;
	try {
		cache_path_ = CreateTempFlowFolder("_trade_flow");
		spdlog::trace("CTPT: trade cache for {} is ready, cache folder name: {}.", id_, cache_path_.string());
	} catch (const FlowFolderCreationError& e) {
		spdlog::error("CTPT: {}", e.what());
		throw(e);
	}
	spdlog::trace("CTPT: Account Initilized.");
}

/// 登出并删除临时文件夹
CTPTradingAccount::~CTPTradingAccount() {
	CTPTradingAccount::LogOffSync();
	DeleteTempFlowFolder(cache_path_);
}

CapitalInfo CTPTradingAccount::Capital() const {
	scoped_lock _{capital_mutex_};
	return capital_;
}
const std::map<InstrumentIndex, HoldingRecord> CTPTradingAccount::holding() const {
	scoped_lock _(holding_mutex_);
	return holding_;
}
const std::vector<TradingRecord> CTPTradingAccount::trades() const {
	scoped_lock _(trades_mutex_);
	return trades_;
}
const std::map<OrderIndex, OrderRecord> CTPTradingAccount::orders() const {
	scoped_lock _(order_mutex_);
	return orders_;
}

// Log on
/**
 * @brief 登录函数
 *
 * 若未登录, 则通过连接, 客户端认证, 账户认证, 确认结算单, 完成登录操作. 之后查询 持仓, 成交, 委托.
 * 开启间隔为60s的自动资金查询线程
 * @exception AuthorizationFailureError 客户端认证失败
 * @exception AccountNumberPasswordError 用户名密码错误
 * @exception NetworkError 网络错误, 无法连接服务器
 * @exception UnknownLoginError 其他未知登录错误
 */
void CTPTradingAccount::LogOnSync() {
	if (is_logged_in()) {
		spdlog::warn("CTPT: Account {} is already logged on.", id_);
		return;
	}
	std::cv_status cv_status;
	{
		unique_lock<mutex> lock(log_on_cv_mutex_);
		LogOnAsync();
		cv_status = log_on_cv_.wait_for(lock, 60s);
	}
	if (cv_status == std::cv_status::timeout) { throw NetworkError(id_); }
	switch (connection_status_) {
		case ConnectionStatus::Done: break;
		case ConnectionStatus::Authorizing: throw AuthorizationFailureError(id_);
		case ConnectionStatus::Logining:
			if (loggin_error_ == LoggingError::FirstLoginPasswordNeedChangeError) {
				throw FirstLoginPasswordNeedChangeError(id_);
			} else if (loggin_error_ == LoggingError::WrongUsernamePasswordError) {
				throw AccountNumberPasswordError(id_);
			} else {
				throw UnknownLoginError(id_);
			}

		default: spdlog::trace("CTPT: connect status: {}", connection_status_); throw UnknownLoginError(id_);
	}

	// initial querying
	QueryPreHolding();
	auto query_func = [this]() {
		while (true) {
			if (connection_status_ == ConnectionStatus::LoggedOut) { return; }
			QueryCapitalSync();
			unique_lock lock(log_on_cv_mutex_);
			log_on_cv_.wait_for(lock, 60s);
		}
	};
	capital_querying_thread_ = std::thread(query_func);
}
void CTPTradingAccount::LogOnAsync() noexcept {
	if (is_logged_in()) {
		spdlog::warn("CTPT: Account({}) is already logged in.", id_);
		return;
	}
	if (papi_ == nullptr) {
		papi_ = CThostFtdcTraderApi::CreateFtdcTraderApi(cache_path_.string().c_str());
		papi_->RegisterSpi(this);
		papi_->SubscribePrivateTopic(THOST_TERT_RESTART);
		papi_->SubscribePublicTopic(THOST_TERT_RESTART);
		for (IPAddress addr : trade_server_addr_) {
			if (addr.substr(4, 2) != "//") { addr = "tcp://" + addr; }
			papi_->RegisterFront(const_cast<char*>(addr.c_str()));
		}
		papi_->Init();
		spdlog::trace("CTPT: {}: Start Logon", id_);
		connection_status_ = ConnectionStatus::Connecting;
	} else {
		PostingLoginRequest();
	}
}

void CTPTradingAccount::OnFrontConnected() {
	spdlog::trace("CTPTS: {}: front connected", id_);
	if (connection_status_ == ConnectionStatus::LoggingOut || connection_status_ == ConnectionStatus::LoggedOut) {
		spdlog::trace("CTPTS: {}: client already logged out!", id_);
		return;
	}
	connection_status_ = ConnectionStatus::Connected;
	CThostFtdcReqAuthenticateField field{};
	strncpy(field.BrokerID, broker_id_.c_str(), sizeof(field.BrokerID));
	strncpy(field.UserID, account_number_.c_str(), sizeof(field.UserID));
	strncpy(field.UserProductInfo, user_product_info_.c_str(), sizeof(field.UserProductInfo));
	strncpy(field.AppID, app_id_.c_str(), sizeof(field.AppID));
	strncpy(field.AuthCode, auth_code_.c_str(), sizeof(field.AuthCode));
	spdlog::trace("ReqAuthenticate: UserProductInfo: {}, AppID: {}, AuthCode: {}", user_product_info_, app_id_,
				  auth_code_);
	spdlog::trace("CTPT: Request authentification.");
	int ret = papi_->ReqAuthenticate(&field, request_id_++);
	RequestSendingConfirm(ret, "Authenticate");
	connection_status_ = ConnectionStatus::Authorizing;
}

void CTPTradingAccount::PostingLoginRequest() {
	CThostFtdcReqUserLoginField loginReq{};
	strncpy(loginReq.BrokerID, broker_id_.c_str(), sizeof(loginReq.BrokerID));
	strncpy(loginReq.UserID, account_number_.c_str(), sizeof(loginReq.UserID));
	strncpy(loginReq.Password, password_.c_str(), sizeof(loginReq.Password));
	strncpy(loginReq.UserProductInfo, account_name_.c_str(), sizeof(loginReq.UserProductInfo));
	int rt = papi_->ReqUserLogin(&loginReq, request_id_++);
	RequestSendingConfirm(rt, "Log on");
	connection_status_ = ConnectionStatus::Logining;
}

void CTPTradingAccount::OnRspAuthenticate(CThostFtdcRspAuthenticateField*, CThostFtdcRspInfoField* pRspInfo, int,
										  bool) {
	if (pRspInfo && pRspInfo->ErrorID == 0) {
		connection_status_ = ConnectionStatus::Authorized;
		spdlog::trace("CTPTS: {}: Authentification successful.", id_);
		PostingLoginRequest();
	} else {
		ErrorResponse(pRspInfo);
		log_on_cv_.notify_one();
		spdlog::error("CTPTS::Authentification FAILED!");
	}
}
void CTPTradingAccount::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo,
									   int, bool) {
	if (!pRspInfo->ErrorID) {
		spdlog::trace("CTPTS: CTP max order ref: {}", pRspUserLogin->MaxOrderRef);
		front_id_ = pRspUserLogin->FrontID;
		session_id_ = pRspUserLogin->SessionID;
		order_ref_ = atol(pRspUserLogin->MaxOrderRef);
		spdlog::info("{}: Loged in successfully!", id_);

		CThostFtdcSettlementInfoConfirmField field{};
		strncpy(field.BrokerID, broker_id_.c_str(), sizeof(field.BrokerID));
		strncpy(field.InvestorID, account_number_.c_str(), sizeof(field.InvestorID));
		spdlog::trace("CTPTS: Requst Settlement Confirmation");
		int ret = papi_->ReqSettlementInfoConfirm(&field, request_id_++);
		RequestSendingConfirm(ret, "Settlement Confirmation");
		connection_status_ = ConnectionStatus::LoggedIn;
	} else {
		switch (pRspInfo->ErrorID) {
			case 3: loggin_error_ = LoggingError::WrongUsernamePasswordError; break;
			case 131: loggin_error_ = LoggingError::WeakPasswordError; break;
			case 140: loggin_error_ = LoggingError::FirstLoginPasswordNeedChangeError; break;
			case 141: loggin_error_ = LoggingError::PasswordOutofDateError; break;
			case 143: loggin_error_ = LoggingError::IPLimitedError; break;
			case 144: loggin_error_ = LoggingError::IPBanedError; break;
			default: loggin_error_ = LoggingError::UnknownLoggingError; break;
		}
		ErrorResponse(pRspInfo);
		spdlog::error("CTPTS: Log in FAILED!");
		log_on_cv_.notify_one();
	}
}
void CTPTradingAccount::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField*,
												   CThostFtdcRspInfoField* pRspInfo, int, bool) {
	if (!pRspInfo->ErrorID) {
		spdlog::trace("CTPTS: Settlement confirmed");
		connection_status_ = ConnectionStatus::Done;
	} else {
		spdlog::error("Settlement Confirmation Failure!!!");
		ErrorResponse(pRspInfo);
	}
	log_on_cv_.notify_one();
}

/// 登出
void CTPTradingAccount::LogOffSync() noexcept {
	if (is_logged_in()) {
		unique_lock<mutex> lock(log_on_cv_mutex_);
		CTPTradingAccount::LogOffAsync();
		log_on_cv_.wait_for(lock, 2s);

		connection_status_ = ConnectionStatus::LoggedOut;
		capital_querying_thread_.join();
		papi_->RegisterSpi(nullptr);
		papi_->Release();
		papi_ = nullptr;
	}
}
void CTPTradingAccount::LogOffAsync() noexcept {
	if (connection_status_ == ConnectionStatus::LoggedOut) {
		spdlog::warn("CTPT: Account({}) already logged out", id_);
		log_on_cv_.notify_all();
		return;
	}
	connection_status_ = ConnectionStatus::LoggingOut;
	CThostFtdcUserLogoutField logoff_info{};
	strncpy(logoff_info.BrokerID, broker_id_.c_str(), sizeof(logoff_info.BrokerID));
	strncpy(logoff_info.UserID, account_number_.c_str(), sizeof(logoff_info.UserID));

	int rt = papi_->ReqUserLogout(&logoff_info, request_id_++);
	RequestSendingConfirm(rt, "Log off");
}
void CTPTradingAccount::OnRspUserLogout(CThostFtdcUserLogoutField*, CThostFtdcRspInfoField* pRspInfo, int, bool) {
	if (pRspInfo->ErrorID) {
		ErrorResponse(pRspInfo);
	} else {
		scoped_lock lock(log_on_cv_mutex_);
		connection_status_ = ConnectionStatus::LoggedOut;
		spdlog::trace("CTPTS: {}: logged out.", id_);
		log_on_cv_.notify_one();
	}
}

bool CTPTradingAccount::UpdatePassword(const Password& new_password) {
	if (new_password == password_) {
		spdlog::warn("CTPTS: {}'s new password is the same as the current one!", id_);
		return true;
	}
	CThostFtdcUserPasswordUpdateField field{};
	strncpy(field.BrokerID, broker_id_.c_str(), sizeof(field.BrokerID));
	strncpy(field.UserID, account_number_.c_str(), sizeof(field.UserID));
	strncpy(field.OldPassword, password_.c_str(), sizeof(field.OldPassword));
	strncpy(field.NewPassword, new_password.c_str(), sizeof(field.NewPassword));
	int rt = papi_->ReqUserPasswordUpdate(&field, request_id_++);
	RequestSendingConfirm(rt, "Requesting password change");
	std::unique_lock lock(log_on_cv_mutex_);
	std::cv_status status = log_on_cv_.wait_for(lock, std::chrono::seconds(2));
	bool ret = status != std::cv_status::timeout && success_;
	if (ret) { password_ = new_password; }
	return ret;
}

void CTPTradingAccount::OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField* pUserPasswordUpdate,
												CThostFtdcRspInfoField* pRspInfo, int, bool) {
	if (pRspInfo->ErrorID == 0) {
		spdlog::info("CTPTS: {} password change successful. old password: '{}', new password: '{}'", id_,
					 pUserPasswordUpdate->OldPassword, pUserPasswordUpdate->NewPassword);
		success_ = true;
	} else {
		ErrorResponse(pRspInfo);
	}
	log_on_cv_.notify_one();
}

// IO
json CTPTradingAccount::CurrentInfoJson() const {
	if (!is_logged_in()) { spdlog::warn("CTPT: {} is not logged in. Exported info would be empty", id_); }

	json ret;
	ret["account_name"] = account_name_;
	ret["broker_name"] = broker_name_;
	ret["account_api"] = "CTP";

	{
		scoped_lock _{capital_mutex_};
		ret["capital"] = capital_;
	}
	{
		scoped_lock _{holding_mutex_};
		ret["holding"] = MapValues(holding_);
	}
	{
		scoped_lock _{trades_mutex_};
		ret["trades"] = trades_;
	}
	{
		scoped_lock _{order_mutex_};
		ret["orders"] = MapValues(orders_);
	}

	ret["commission_rate"] = MapValues(instrument_commission_rate_);
	return ret;
}

/**
 * @brief 查询市场上的所有合约
 * @exception NetworkError 网络错误, 无法连接服务器
 */
map<Ticker, InstrumentInfo> CTPTradingAccount::QueryInstruments() {
	CThostFtdcQryInstrumentField field{};
	std::cv_status cv_status;
	{
		unique_lock cv_lock(query_mutex_);
		rate_throttler_.wait();
		int ret = papi_->ReqQryInstrument(&field, request_id_++);
		RequestSendingConfirm(ret, "Query Instruments");
		cv_status = query_cv_.wait_for(cv_lock, 60s);
	}
	if (cv_status == std::cv_status::timeout) { throw NetworkError(id_); }
	spdlog::trace("CTPTS: {}: Acquired all instruments.", id_);
	return instrument_info_;
}
void CTPTradingAccount::OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField*, int,
										   bool bIsLast) {
	if (pInstrument == nullptr) {
		spdlog::error("QueryInstruments returns a null pointer. The server maybe ill-configed.");
		std::terminate();
	}
	if ((pInstrument->ProductClass == THOST_FTDC_PC_Futures) ||
		(pInstrument->ProductClass == THOST_FTDC_PC_SpotOption)) {
		InstrumentInfo info = TranslateInstrumentInfo(pInstrument);
		Ticker id = info.instrument_id;
		std::ranges::transform(id, id.begin(), ::toupper);
		instrument_info_.insert({id, std::move(info)});
	}
	if (bIsLast) { query_cv_.notify_one(); }
}

std::map<Ticker, InstrumentCommissionRate> CTPTradingAccount::QueryCommissionRate() {
	TestQueryRequestsPerSecond();
	for (auto& [instrument_index, instrument_info] : instrument_info_) {
		QueryCommissionRate(instrument_info.instrument_id, instrument_info.instrument_type);
	}
	return instrument_commission_rate_;
}
/**
 * @brief 查询交易手续费
 * @param ticker 合约代码
 * @param instrument_type 合约品种. 支持`InstrumentType::Future`和`InstrumentType::Option`
 * @return 该合约的手续费
 * @exception NetworkError 网络错误, 无法连接服务器
 */
InstrumentCommissionRate CTPTradingAccount::QueryCommissionRate(const Ticker& ticker, InstrumentType instrument_type) {
	switch (instrument_type) {
		case InstrumentType::Future: {
			CThostFtdcQryInstrumentCommissionRateField a{};
			strncpy(a.BrokerID, broker_id_.c_str(), sizeof(a.BrokerID));
			strncpy(a.InvestorID, account_number_.c_str(), sizeof(a.InvestorID));
			strncpy(a.InstrumentID, ticker.c_str(), sizeof(a.InstrumentID));
			{
				unique_lock lock(query_mutex_);
				rate_throttler_.wait();
				int rt = papi_->ReqQryInstrumentCommissionRate(&a, request_id_++);
				RequestSendingConfirm(rt, "Querying future commission rate");
				query_cv_.wait_for(lock, 2s);
			}
			break;
		}
		case InstrumentType::Option: {
			CThostFtdcQryOptionInstrCommRateField field{};
			strncpy(field.BrokerID, broker_id_.c_str(), sizeof(field.BrokerID));
			strncpy(field.InvestorID, account_number_.c_str(), sizeof(field.InvestorID));
			strncpy(field.InstrumentID, ticker.c_str(), sizeof(field.InstrumentID));
			{
				unique_lock lock(query_mutex_);
				rate_throttler_.wait();
				int rt = papi_->ReqQryOptionInstrCommRate(&field, request_id_++);
				RequestSendingConfirm(rt, "Querying option commission rate");
				query_cv_.wait_for(lock, 2s);
			}
			break;
		}
		default: throw(UnknownReturnDataError());
	}
	if (instrument_commission_rate_.contains(ticker)) {
		return instrument_commission_rate_.at(ticker);
	} else {
		return instrument_commission_rate_.at(GetProductID(ticker));
	}
}
void CTPTradingAccount::OnRspQryInstrumentCommissionRate(
	CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate, CThostFtdcRspInfoField*, int, bool bIsLast) {
	if (pInstrumentCommissionRate) {
		string id = pInstrumentCommissionRate->InstrumentID;
		InstrumentCommissionRate rate{
			.instrument_id = id,
			.open_ratio_by_money = pInstrumentCommissionRate->OpenRatioByMoney,
			.open_ratio_by_volume = pInstrumentCommissionRate->OpenRatioByVolume,
			.close_ratio_by_money = pInstrumentCommissionRate->CloseRatioByMoney,
			.close_ratio_by_volume = pInstrumentCommissionRate->CloseRatioByVolume,
			.close_today_ratio_by_money = pInstrumentCommissionRate->CloseTodayRatioByMoney,
			.close_today_ratio_by_volume = pInstrumentCommissionRate->CloseTodayRatioByVolume,
		};
		instrument_commission_rate_.insert({id, std::move(rate)});
		spdlog::trace("CTPTS: {}'s commission rate info received.", id);
	}
	if (bIsLast) { query_cv_.notify_one(); }
}
void CTPTradingAccount::OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField* pOptionInstrCommRate,
													CThostFtdcRspInfoField*, int, bool bIsLast) {
	if (pOptionInstrCommRate) {
		string id = pOptionInstrCommRate->InstrumentID;
		InstrumentCommissionRate rate{
			.instrument_id = id,
			.open_ratio_by_money = pOptionInstrCommRate->OpenRatioByMoney,
			.open_ratio_by_volume = pOptionInstrCommRate->OpenRatioByVolume,
			.close_ratio_by_money = pOptionInstrCommRate->CloseRatioByMoney,
			.close_ratio_by_volume = pOptionInstrCommRate->CloseRatioByVolume,
			.close_today_ratio_by_money = pOptionInstrCommRate->CloseTodayRatioByMoney,
			.close_today_ratio_by_volume = pOptionInstrCommRate->CloseTodayRatioByVolume,
		};
		instrument_commission_rate_.insert({id, std::move(rate)});
		spdlog::trace("CTPTS: {}'s commission rate info received.", id);
	}
	if (bIsLast) { query_cv_.notify_one(); }
}

/// 查询资金情况
void CTPTradingAccount::QueryCapitalSync() {
	scoped_lock _(query_mutex_);
	unique_lock l(capital_mutex_);
	QueryCapitalAsync();
	query_cv_.wait_for(l, 1s);
}
void CTPTradingAccount::QueryCapitalAsync() {
	CThostFtdcQryTradingAccountField account_info{};
	strncpy(account_info.BrokerID, broker_id_.c_str(), sizeof(account_info.BrokerID));
	strncpy(account_info.InvestorID, account_number_.c_str(), sizeof(account_info.InvestorID));
	strncpy(account_info.CurrencyID, "CNY", sizeof(account_info.CurrencyID));

	rate_throttler_.wait();
	int code = papi_->ReqQryTradingAccount(&account_info, request_id_++);
	RequestSendingConfirm(code, "Querying capital");
}
void CTPTradingAccount::OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField*,
											   int, bool) {
	{
		scoped_lock _(capital_mutex_);
		capital_ = CapitalInfo{
			.balance = pTradingAccount->Balance,
			.margin_used = pTradingAccount->CurrMargin,
			.available = pTradingAccount->Balance - pTradingAccount->CurrMargin,
			.commission = pTradingAccount->Commission,
			.withdraw_allowance = pTradingAccount->WithdrawQuota,
		};
		spdlog::trace("CTPTS: {}: balance: {:.2f}, margin: {:.2f}, conmmission: {:.2f}", id_, capital_.balance,
					  capital_.margin_used, capital_.commission);
	}
	query_cv_.notify_one();
}

/**
 * @brief 更新昨持仓情况
 * @exception NetworkError 网络错误, 无法连接服务器
 */
void CTPTradingAccount::QueryPreHolding() {
	CThostFtdcQryInvestorPositionField investor_info{};
	strncpy(investor_info.BrokerID, broker_id_.c_str(), sizeof(investor_info.BrokerID));
	strncpy(investor_info.InvestorID, account_number_.c_str(), sizeof(investor_info.InvestorID));

	std::cv_status status;
	{
		unique_lock lock(holding_mutex_);
		rate_throttler_.wait();
		int code = papi_->ReqQryInvestorPosition(&investor_info, request_id_++);
		RequestSendingConfirm(code, "Querying holding");
		status = query_cv_.wait_for(lock, 60s);
	}

	if (status == std::cv_status::timeout) {
		spdlog::error("CTPTS: {}: querying holding failed after waiting 60s", id_);
		throw NetworkError(id_);
	}
}
void CTPTradingAccount::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition,
												 CThostFtdcRspInfoField*, int, bool bIsLast) {
	spdlog::trace("CTPTS: Position Accquired.");
	if (pInvestorPosition && (pInvestorPosition->YdPosition != 0) &&
		(string(pInvestorPosition->InstrumentID).find("SP") != 0)) {
		scoped_lock _(holding_mutex_);
		string instrument_id = pInvestorPosition->InstrumentID;
		Direction direction =
			(pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long ? Direction::Long : Direction::Short);
		HedgeFlagType hedge_flag = kHedgeFlagTranslator.at(pInvestorPosition->HedgeFlag);
		InstrumentIndex index{instrument_id, direction, hedge_flag};
		if (holding_.contains(index)) {
			auto& loc = holding_.at(index);
			loc.total_quantity += pInvestorPosition->YdPosition;
			loc.pre_quantity += pInvestorPosition->YdPosition;
		} else {
			HoldingRecord rec{
				.exchange = kExchangeTranslator.at(pInvestorPosition->ExchangeID),
				.instrument_id = instrument_id,
				.direction = direction,
				.hedge_flag = hedge_flag,
				.total_quantity = pInvestorPosition->YdPosition,
				.today_quantity = 0,
				.pre_quantity = pInvestorPosition->YdPosition,
			};
			holding_.insert({index, rec});
		}
	}
	if (bIsLast) {
		query_cv_.notify_one();
		spdlog::trace("CTPTS: {}: Acquired all position.", id_);
	}
}

/// 接收成交情况
void CTPTradingAccount::OnRtnTrade(CThostFtdcTradeField* pTrade) {
	spdlog::trace("CTPTS: New return trade.");
	auto trade = TradeField2TradingRecord(pTrade);

	scoped_lock _(trades_mutex_, holding_mutex_);
	trades_.push_back(trade);

	if (trade.open_close != OpenCloseType::Open) { trade.direction = ReverseDirection(trade.direction); }

	InstrumentIndex index{trade.instrument_id, trade.direction, trade.hedge_flag};
	if (holding_.contains(index)) {
		auto& loc = holding_.at(index);
		loc.total_quantity += EnumToPositiveOrNegative<OpenCloseType>(trade.open_close) * trade.volume;

		switch (trade.open_close) {
			case OpenCloseType::Open: {
				loc.today_quantity += trade.volume;
				break;
			}
			case OpenCloseType::CloseToday: {
				// 上期所、能源中心的平今仓按今仓先开先平顺序对持仓明细进行平仓处理，平仓指令按昨仓先开先平顺序对持仓明细进行平仓处理。
				loc.today_quantity -= trade.volume;
				break;
			}
			case OpenCloseType::CloseYesterday: {
				// 上期所、能源中心的平今仓按今仓先开先平顺序对持仓明细进行平仓处理，平仓指令按昨仓先开先平顺序对持仓明细进行平仓处理。
				loc.pre_quantity -= trade.volume;
				break;
			}
			case OpenCloseType::Close: {
				switch (trade.exchange) {
					case Exchange::DCE: {
						// 大连，如果当日有开仓，则先平今再平昨。
						Volume vol = std::max(loc.today_quantity, trade.volume);
						loc.today_quantity -= vol;
						loc.pre_quantity -= (trade.volume - vol);
						break;
					}
					case Exchange::CFE:
						[[fallthrough]];
					case Exchange::CZC: {
						// 中金所，郑商所: 先开先平。
						Volume vol = std::max(loc.pre_quantity, trade.volume);
						loc.pre_quantity -= vol;
						loc.today_quantity -= (trade.volume - vol);
						break;
					}
					default: break;
				}
				break;
			}
			default: break;
		}
	} else {
		// new openings
		HoldingRecord rec{.exchange = trade.exchange,
						  .instrument_id = trade.instrument_id,
						  .direction = trade.direction,
						  .hedge_flag = trade.hedge_flag,
						  .total_quantity = trade.volume,
						  .today_quantity = trade.volume,
						  .pre_quantity = 0};
		holding_.emplace(index, rec);
	}
}

void CTPTradingAccount::FilterCancelableOrders(map<OrderIndex, OrderRecord>::iterator& loc) {
	switch (loc->second.order_status) {
		case OrderStatus::AllTraded:		   // 全部成交
		case OrderStatus::Canceled:			   // 撤单
		case OrderStatus::RejectedByServer:	   // CTP拒绝此订单
		case OrderStatus::RejectedByExchange:  // 交易所拒绝此订单
		case OrderStatus::NotTouched:		   // 尚未触发
		case OrderStatus::Touched:			   // 已触发
		case OrderStatus::Unknown:			   // 未知
			cancelable_orders_.erase(loc->first);
			break;
		case OrderStatus::PartTradedQueueing:	  // 部分成交还在队列中
		case OrderStatus::PartTradedNotQueueing:  // 部分成交不在队列中
		case OrderStatus::NoTradeQueueing:		  // 未成交还在队列中
		case OrderStatus::NoTradeNotQueueing:	  // 未成交不在队列中
			cancelable_orders_.insert(loc->first);
			break;
	}
}
/// 接收委托记录
void CTPTradingAccount::OnRtnOrder(CThostFtdcOrderField* pOrder) {
	spdlog::trace("CTPTS: Order Aquired.");
	OrderRef order_ref = atol(pOrder->OrderRef);
	OrderIndex index{pOrder->FrontID, pOrder->SessionID, order_ref};
	{
		scoped_lock lock(order_mutex_);
		auto loc = orders_.find(index);
		if (loc == orders_.end()) {
			spdlog::trace("SPI: New Order Record Received.");
			std::tie(loc, std::ignore) = orders_.emplace(index, OrderField2OrderRecord(pOrder));
			order_cv_.notify_one();
		} else {
			if (pOrder->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected) {
				orders_[index].order_status = OrderStatus::RejectedByExchange;
				spdlog::error("Order(ref: {}) was rejected by exchange. error message: {}", order_ref,
							  GB2312ToUTF8(pOrder->StatusMsg));
				order_cv_.notify_one();
			} else {
				spdlog::trace("SPI: Existing Order Record Status Updated.");
				loc->second.order_status = kOrderStatusTranslator.at(pOrder->OrderStatus);
				loc->second.remained_volume = pOrder->VolumeTotal;
				loc->second.traded_volume = pOrder->VolumeTraded;
			}
		}
		FilterCancelableOrders(loc);
	}
	spdlog::trace("CTPTS: Return Order processed.");
}

// Insert orders
CThostFtdcInputOrderField CTPTradingAccount::NativeOrder2CTPOrder(const Order& order) const {
	CThostFtdcInputOrderField field{};
	strncpy(field.BrokerID, broker_id_.c_str(), sizeof(field.BrokerID));
	strncpy(field.InvestorID, account_number_.c_str(), sizeof(field.InvestorID));
	strncpy(field.UserID, account_number_.c_str(), sizeof(field.UserID));
	strncpy(field.InstrumentID, order.instrument_id.c_str(), sizeof(field.InstrumentID));
	strncpy(field.ExchangeID, kExchangeTranslator.at(order.exchange).c_str(), sizeof(field.ExchangeID));
	field.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	field.MinVolume = 1;
	field.IsAutoSuspend = 0;

	field.CombHedgeFlag[0] = kHedgeFlagTranslator.at(order.hedge_flag);
	field.Direction = (order.direction == Direction::Long) ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
	field.ContingentCondition = kContingentConditionTranslator.at(order.contingent_condition);
	switch (order.time_in_force) {
		case TimeInForce::GFD:
			field.TimeCondition = THOST_FTDC_TC_GFD;
			field.VolumeCondition = THOST_FTDC_VC_AV;
			break;
		case TimeInForce::FAK:
			field.TimeCondition = THOST_FTDC_TC_IOC;
			field.VolumeCondition = THOST_FTDC_VC_AV;
			break;
		case TimeInForce::FOK:
			field.TimeCondition = THOST_FTDC_TC_IOC;
			field.VolumeCondition = THOST_FTDC_VC_CV;
			break;
	}

	field.OrderPriceType = kOrderPriceTypeTranslator.at(order.order_price_type);
	field.CombOffsetFlag[0] = kOpenCloseTranslator.at(order.open_close);
	field.LimitPrice = order.limit_price;
	field.VolumeTotalOriginal = order.volume;
	return field;
}
OrderIndex CTPTradingAccount::PlaceOrderASync(CThostFtdcInputOrderField& field) {
	OrderRef local_order_ref = ++order_ref_;
	std::to_chars(field.OrderRef, field.OrderRef + sizeof(field.OrderRef), static_cast<OrderRef>(local_order_ref));
	int ret = papi_->ReqOrderInsert(&field, request_id_++);
	RequestSendingConfirm(ret, "Order Insert");
	spdlog::trace("CTPTS: Order {} request comptlete.", local_order_ref);
	OrderIndex index{front_id_, session_id_, local_order_ref};
	return index;
}
OrderIndex CTPTradingAccount::PlaceOrderASync(Order order) {
	CThostFtdcInputOrderField ctp_order = NativeOrder2CTPOrder(order);
	return PlaceOrderASync(ctp_order);
}
/**
 * @brief 下单函数
 * @param order 订单
 * @exception OrderInfoError 订单信息错误
 */
void CTPTradingAccount::PlaceOrderSync(Order order) {
	OrderIndex index;
	{
		unique_lock<mutex> lock(order_sync_mutex_);
		index = PlaceOrderASync(order);
		order_cv_.wait_for(lock, 2s);
	}

	scoped_lock lock(order_mutex_);
	auto loc = orders_.find(index);
	if (loc == orders_.end()) {
		throw OrderInfoError(account_name_, "Order rejected by broker.");
	} else if (loc->second.order_status == OrderStatus::RejectedByExchange) {
		throw OrderInfoError(account_name_, "Order rejected by exchange.");
	}
}
/**
 * @brief 批量下单函数
 * @param orders 订单
 * @exception OrderInfoError 订单信息错误
 */
vector<OrderIndex> CTPTradingAccount::BatchOrderSync(std::vector<Order> orders) {
	vector<OrderIndex> ret;
	unique_lock<mutex> lock(order_sync_mutex_);
	for (const Order& order : orders) {
		CThostFtdcInputOrderField field = NativeOrder2CTPOrder(order);
		OrderIndex index = PlaceOrderASync(field);
		ret.push_back(index);
	}
	order_cv_.wait_for(lock, 2s, [&]() { return orders_.find(ret.back()) != orders_.end(); });
	return ret;
}

map<OrderIndex, OrderStatus> CTPTradingAccount::GetBatchOrderStatus(std::vector<OrderIndex> indexes) {
	map<OrderIndex, OrderStatus> ret;
	scoped_lock lock(order_mutex_);
	for (const OrderIndex& index : indexes) {
		auto loc = orders_.find(index);
		if (loc == orders_.end()) {
			throw OrderInfoError(account_name_, "Order rejected by broker.");
		} else {
			ret[index] = loc->second.order_status;
		}
	}
	return ret;
}

void CTPTradingAccount::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int,
										 bool) {
	if (pRspInfo->ErrorID) {
		ErrorResponse(pRspInfo);
		order_cv_.notify_one();
	} else {
		spdlog::trace("CTPTS: Order {} accepted by borker.", pInputOrder->OrderRef);
	}
}

/**
 * @brief 取消订单
 * @param index 订单号
 * @exception OrderRefError 订单号不存在错误
 */
void CTPTradingAccount::CancelOrder(OrderIndex index) {
	if (!orders_.contains(index)) { throw OrderRefError(account_name_); }
	const OrderRecord& rec = orders_.at(index);
	CThostFtdcInputOrderActionField field{};
	strncpy(field.BrokerID, broker_id_.c_str(), sizeof(field.BrokerID));
	strncpy(field.InvestorID, account_number_.c_str(), sizeof(field.InvestorID));
	strncpy(field.UserID, account_number_.c_str(), sizeof(field.UserID));
	strncpy(field.InstrumentID, rec.instrument_id.c_str(), sizeof(field.InstrumentID));
	strncpy(field.ExchangeID, kExchangeTranslator.at(rec.exchange).c_str(), sizeof(field.ExchangeID));
	field.FrontID = rec.front_id;
	field.SessionID = rec.session_id;
	std::to_chars(field.OrderRef, field.OrderRef + sizeof(field.OrderRef), static_cast<OrderRef>(order_ref_));
	field.ActionFlag = THOST_FTDC_AF_Delete;
	int ret = papi_->ReqOrderAction(&field, request_id_++);
	RequestSendingConfirm(ret, "Cancel order");
}
void CTPTradingAccount::CancelAllPendingOrders() {
	for (auto& pending_order : cancelable_orders_) { CancelOrder(pending_order); }
}

void CTPTradingAccount::TestQueryRequestsPerSecond() {
	// assuming:
	// 1. QueryInstruments can be done in less than a second.
	// 2. commission rate querying can be done in less than a second.
	QueryInstruments();
	const auto& ticker_info = instrument_info_.cbegin()->second;
	CThostFtdcQryInstrumentCommissionRateField a{};
	strncpy(a.BrokerID, broker_id_.c_str(), sizeof(a.BrokerID));
	strncpy(a.InvestorID, account_number_.c_str(), sizeof(a.InvestorID));
	strncpy(a.InstrumentID, ticker_info.instrument_id.c_str(), sizeof(a.InstrumentID));

	int count = 1;
	bool looping = true;
	while (looping) {
		{
			unique_lock lock(query_mutex_);
			rate_throttler_.wait();
			int rt = papi_->ReqQryInstrumentCommissionRate(&a, request_id_++);
			if (rt == 0) {
				++count;
			} else {
				looping = false;
			}
			query_cv_.wait_for(lock, 2s);
		}
	}
	rate_throttler_ = RateThrottler<std::chrono::seconds>{count, std::chrono::seconds(1)};
}

#include "ctpmarketdata.h"

#include <chrono>

#include <spdlog/spdlog.h>

#include "ctp_utils.h"
#include "utsexceptions.h"

using std::vector, std::string, std::cv_status, std::lock_guard, std::mutex, std::unique_lock;
using namespace std::chrono_literals;

/**
 * @brief 构造CTPMarketData
 * @details 构造期间会在系统临时文件夹创建一个缓存文件.
 * @param server_addr CTP行情服务器地址. 需注明协议和端口, 如: "ctp://1.2.3.4:5678"
 * @exception FlowFolderCreationError 临时文件夹无法建立时抛出
 */
CTPMarketDataBase::CTPMarketDataBase(const vector<IPAddress>& server_addr) : MarketDataSource(server_addr) {
	try {
		cache_path_ = CreateTempFlowFolder("_md_flow");
		spdlog::trace("CTPM: market data cache is ready, cache folder name: {}.", cache_path_.string());
	} catch (const std::exception& e) {
		spdlog::error("CTPM: market data cache fail to initilize");
		throw(e);
	}
}

CTPMarketDataBase::~CTPMarketDataBase() {
	if (subscribed_tickers_.size() != 0) {
		vector<Ticker> tickers(subscribed_tickers_.begin(), subscribed_tickers_.end());
		try {
			CTPMarketDataBase::Unsubscribe(tickers);
		} catch (const std::exception&) {}
	}
	CTPMarketDataBase::LogOut();
	DeleteTempFlowFolder(cache_path_);
}

/**
 * @brief 登录
 * @exception NetworkError 网络错误, 无法连接服务器
 * @exception LoginError 登录失败
 */
void CTPMarketDataBase::LogIn() {
	md_api_ = CThostFtdcMdApi::CreateFtdcMdApi(cache_path_.string().c_str());
	md_api_->RegisterSpi(this);
	for (IPAddress& addr : server_addr_) {
		if (addr.substr(4, 2) != "//") { addr = "tcp://" + addr; }
		md_api_->RegisterFront(const_cast<char*>(addr.c_str()));
	}

	cv_status cv_status;
	{
		unique_lock lock(query_mutex_);
		md_api_->Init();
		cv_status = cv_.wait_for(lock, 5s);
	}
	if (cv_status == std::cv_status::timeout) {
		throw NetworkError("Market info");
	} else if (status_ == ConnectionStatus::Connected) {
		spdlog::info("CTPM: CTP market data server login successful.");
	} else {
		spdlog::info("CTPM: Failed to log in CTP market data server.");
		throw LoginError();
	}
}
void CTPMarketDataBase::OnFrontConnected() {
	CThostFtdcReqUserLoginField reqUserLogin{};
	md_api_->ReqUserLogin(&reqUserLogin, request_id_++);
}
void CTPMarketDataBase::OnRspUserLogin(CThostFtdcRspUserLoginField*, CThostFtdcRspInfoField* pRspInfo, int, bool) {
	if (!pRspInfo->ErrorID) {
		spdlog::trace("CTPMS: Log in successful!");
		status_ = ConnectionStatus::Connected;
	} else {
		ErrorResponse(pRspInfo);
	}
	cv_.notify_one();
}

/**
 * @brief 登出
 */
void CTPMarketDataBase::LogOut() noexcept {
	if (is_logged_in()) {
		CThostFtdcUserLogoutField a{};
		unique_lock lock(query_mutex_);
		md_api_->ReqUserLogout(&a, request_id_++);
		cv_.wait_for(lock, 2s);
		status_ = ConnectionStatus::Disconnected;
		spdlog::debug("CTPM:Log off market info.");

		md_api_->RegisterSpi(nullptr);
		md_api_->Release();
		md_api_ = nullptr;
	}
}
void CTPMarketDataBase::OnRspUserLogout(CThostFtdcUserLogoutField*, CThostFtdcRspInfoField* pRspInfo, int, bool) {
	if (pRspInfo->ErrorID) {
		ErrorResponse(pRspInfo);
		spdlog::error("CTPMS: Log out FAILED!");
	} else {
		spdlog::trace("CTPMS: logged out.");
	}
	cv_.notify_one();
}

/**
 * @brief 订阅合约
 * @param ticker_list 新订阅的Ticker序列
 * @note 该函数每100个ticker发送一次订阅请求.
 * @exception NetworkError 网络错误, 无法连接服务器
 */
void CTPMarketDataBase::Subscribe(const vector<Ticker>& ticker_list) {
	size_t count = 0;
	size_t list_size = ticker_list.size();
	char** instruments = new char*[list_size];
	for (const auto& ticker : ticker_list) {
		if (!subscribed_tickers_.contains(ticker)) {
			instruments[count] = const_cast<char*>(ticker.c_str());
			++count;
		}
	}
	spdlog::trace("CTPM: Asked to subscribe {} ticker, {} not in current subscribe list", list_size, count);

	if (count > 0) {
		unique_lock lock(query_mutex_);
		const size_t increment = 100;
		for (size_t start_index = 0; start_index < count; start_index += increment) {
			size_t num = std::min(increment, count - start_index);
			int result = md_api_->SubscribeMarketData(instruments + start_index, static_cast<int>(num));
			RequestSendingConfirm(result, "Subscribe market data");
			cv_status status = cv_.wait_for(lock, 2s);
			if (status == std::cv_status::timeout) {
				spdlog::error("fail to subscribe tickers");
				delete[] instruments;
				throw NetworkError("Market data");
			}
		}
		spdlog::trace("CTPM: CTPMarketData: request to subscribe {} contracts, {} contracts currently subscribed",
					  count, subscribed_tickers_.size());
	}
	delete[] instruments;
}
void CTPMarketDataBase::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument,
										   CThostFtdcRspInfoField*, int, bool bIsLast) {
	subscribed_tickers_.insert(Ticker(pSpecificInstrument->InstrumentID));
	if (bIsLast) { cv_.notify_one(); }
}

/**
 * @brief 取消合约订阅
 * @param ticker_list 新订阅的Ticker序列
 * @note 该函数每100个ticker发送一次取消订阅请求.
 * @exception NetworkError 网络错误, 无法连接服务器
 */
void CTPMarketDataBase::Unsubscribe(const vector<Ticker>& ticker_list) {
	size_t count = 0;
	size_t list_size = ticker_list.size();
	char** instruments = new char*[list_size];
	for (const auto& ticker : ticker_list) {
		if (subscribed_tickers_.contains(ticker)) {
			instruments[count] = const_cast<char*>(ticker.c_str());
			++count;
		}
	}
	spdlog::trace("CTPM: Asked to unsubscribe {} ticker, {} in current subscribe list", list_size, count);
	if (count > 0) {
		unique_lock lock(query_mutex_);
		const size_t increment = 100;
		for (size_t start_index = 0; start_index < count; start_index += increment) {
			size_t num = std::min(increment, count - start_index);
			int result = md_api_->UnSubscribeMarketData(instruments + start_index, static_cast<int>(num));
			RequestSendingConfirm(result, "Unsubscribe market data");
			cv_status status = cv_.wait_for(lock, 2s);
			if (status == std::cv_status::timeout) {
				spdlog::error("CTPM: fail to unsubscribe tickers");
				delete[] instruments;
				throw NetworkError("Market data");
			} else {
				spdlog::trace("CTPM: Unsubscribe tickers successful.");
			}
		}
	}
	delete[] instruments;
}

void CTPMarketDataBase::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument,
											 CThostFtdcRspInfoField*, int, bool bIsLast) {
	Ticker ticker(pSpecificInstrument->InstrumentID);
	subscribed_tickers_.erase(ticker);
	market_data_.erase(ticker);
	if (bIsLast) { cv_.notify_one(); }
}

void CTPMarketData::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) {
	MarketDepth md = CTPMarketData2MarketDepth(pDepthMarketData);
	market_data_[md.instrument_id] = md;
}

MarketDepth CTPMarketData::CTPMarketData2MarketDepth(CThostFtdcDepthMarketDataField* pDepthMarketData) {
	MarketDepth md;

	auto id = pDepthMarketData->InstrumentID;
	md.instrument_id = id;

	string dt(pDepthMarketData->ActionDay);
	md.update_time = dt.substr(0, 4) + '-' + dt.substr(4, 2) + '-' + dt.substr(6, 2) + ' ' +
					 pDepthMarketData->UpdateTime + '.' + std::to_string(pDepthMarketData->UpdateMillisec);

	md.settle = SanitizeData(pDepthMarketData->SettlementPrice);
	md.average_price = pDepthMarketData->AveragePrice;
	md.upper_limit = pDepthMarketData->UpperLimitPrice;
	md.lower_limit = pDepthMarketData->LowerLimitPrice;

	double open = pre_latest_[id];
	double close = pDepthMarketData->LastPrice;
	double high =
		pDepthMarketData->HighestPrice > pre_high_[id] ? pDepthMarketData->HighestPrice : std::max(open, close);
	double low = pDepthMarketData->LowestPrice < pre_low_[id] ? pDepthMarketData->LowestPrice : std::min(open, close);
	double turnover = pDepthMarketData->Turnover - pre_turnover_[id];
	int volume = pDepthMarketData->Volume - pre_volume_[id];

	pre_latest_[id] = close;
	pre_high_[id] = pDepthMarketData->HighestPrice;
	pre_low_[id] = pDepthMarketData->LowestPrice;
	pre_turnover_[id] = pDepthMarketData->Turnover;
	pre_volume_[id] = pDepthMarketData->Volume;

	md.ohlclvt = {.open = SanitizeData(open),
				  .high = SanitizeData(high),
				  .low = SanitizeData(low),
				  .close = SanitizeData(pDepthMarketData->ClosePrice),
				  .last = pDepthMarketData->LastPrice,
				  .volume = volume,
				  .turnover = turnover};
	md.open_interest = static_cast<int>(pDepthMarketData->OpenInterest),
	md.delta = SanitizeData(pDepthMarketData->CurrDelta);
	md.ask = {
		PriceVolume{SanitizeData(pDepthMarketData->AskPrice1), pDepthMarketData->AskVolume1},
		PriceVolume{SanitizeData(pDepthMarketData->AskPrice2), pDepthMarketData->AskVolume2},
		PriceVolume{SanitizeData(pDepthMarketData->AskPrice3), pDepthMarketData->AskVolume3},
		PriceVolume{SanitizeData(pDepthMarketData->AskPrice4), pDepthMarketData->AskVolume4},
		PriceVolume{SanitizeData(pDepthMarketData->AskPrice5), pDepthMarketData->AskVolume5},
	};
	md.bid = {
		PriceVolume{SanitizeData(pDepthMarketData->BidPrice1), pDepthMarketData->BidVolume1},
		PriceVolume{SanitizeData(pDepthMarketData->BidPrice2), pDepthMarketData->BidVolume2},
		PriceVolume{SanitizeData(pDepthMarketData->BidPrice3), pDepthMarketData->BidVolume3},
		PriceVolume{SanitizeData(pDepthMarketData->BidPrice4), pDepthMarketData->BidVolume4},
		PriceVolume{SanitizeData(pDepthMarketData->BidPrice5), pDepthMarketData->BidVolume5},
	};
	return md;
}

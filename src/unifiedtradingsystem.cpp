#include "unifiedtradingsystem.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <thread>

#include <spdlog/spdlog.h>

#include "ctpmarketdata.h"
#include "ctptradingaccount.h"
#include "enum_utils.h"
#include "trading_utils.h"
#include "utsexceptions.h"
#include "version.h"

using std::map, std::set, std::vector, std::string;

UnifiedTradingSystem::UnifiedTradingSystem() {
#ifdef NDEBUG
	spdlog::set_level(spdlog::level::info);
#else
	spdlog::set_level(spdlog::level::trace);
#endif
	spdlog::set_pattern("[%H:%M:%S:%e] [thread %6t] [%^---%L---%$] %v");
	spdlog::flush_every(std::chrono::seconds(1));
}
UnifiedTradingSystem::~UnifiedTradingSystem() { LogOut(); }

bool UnifiedTradingSystem::empty() const { return accounts_.empty(); }

size_t UnifiedTradingSystem::size() const { return accounts_.size(); }

std::vector<Account> UnifiedTradingSystem::available_accounts() const {
	std::vector<Account> ret;
	for (auto& [account_index, account] : accounts_) { ret.push_back(account_index); }
	return ret;
}

TradingAccount* UnifiedTradingSystem::GetHandle(const Account& account) { return accounts_.at(account); }

void UnifiedTradingSystem::AddBroker(const BrokerInfo& broker_info) {
	broker_info_[broker_info.broker_name] = broker_info;
}

void UnifiedTradingSystem::AddMarketDataSource(const vector<IPAddress>& server_addr) {
	market_data_source_ = new CTPMarketData(server_addr);
	market_data_ = &market_data_source_->market_data();
}

/**
 * @brief 添加账户信息
 * @param account_info 账户信息
 * @pre 对应的经纪商信息需提前通过 `AddBroker` 添加.
 */
void UnifiedTradingSystem::AddAccount(const AccountInfo& account_info) {
	if (!account_info.enable) { return; }
	if (!broker_info_.contains(account_info.broker_name)) {
		spdlog::error("{}'s server info could not be found.", account_info.broker_name);
		throw IncompleteBrokerInfoError(account_info.broker_name);
	}

	BrokerInfo& broker = broker_info_.at(account_info.broker_name);
	switch (broker.API_type) {
		case APIType::CTP:
			try {
				accounts_[{account_info.account_name, account_info.broker_name}] =
					new CTPTradingAccount(account_info, broker);
				spdlog::info("Account {} - {} added.", account_info.account_name, account_info.broker_name);
			} catch (FlowFolderCreationError& error) { spdlog::error(error.what()); } catch (...) {
				spdlog::error("Unknown error while adding account {} - {} to system, exit!", account_info.account_name,
							  account_info.broker_name);
			}
	}
}
/// 添加经纪商信息
void UnifiedTradingSystem::AddBroker(const vector<BrokerInfo>& broker_info) {
	for (auto& broker : broker_info) broker_info_[broker.broker_name] = broker;
}

void UnifiedTradingSystem::AddAccount(const std::vector<AccountInfo>& accounts_info) {
	for (auto& account : accounts_info) AddAccount(account);
}

void UnifiedTradingSystem::setNoCloseTodayTickers(const std::set<Ticker>& no_close_today_tickers) {
	no_close_today_tickers_ = no_close_today_tickers;
}
/**
 * @brief 通过Json文件初始化
 *
 * 可通过 `/src/generate_uts_config.py` 和 `config_example.xlsx` 生成适配文件
 * @param config Json适配文件
 */
void UnifiedTradingSystem::InitFromJsonConfig(json config) {
	auto account_info = config.at("accounts").get<vector<AccountInfo>>();
	auto broker_info = config.at("brokers").get<vector<BrokerInfo>>();
	auto market_server = config.at("md_server").get<map<ServerName, IPAddress>>();
	vector<IPAddress> md_server = MapValues(market_server);

	AddMarketDataSource(md_server);
	AddBroker(broker_info);
	AddAccount(account_info);
}
/**
 * @brief 通过 `UTSConfigDB` 完成初始化
 *
 * 可通过 `/src/generate_utsioo_config.py` 和 `config_example.xlsx` 生成适配文件
 * 样本适配文件可在 `tests/test_files/sample_db.sqlite3` 中找到 `UTSConfigDB` 实例化.
 * @param config UTSConfigDB
 */
void UnifiedTradingSystem::InitFromDBConfig(const UTSConfigDB& config) {
	broker_info_ = config.GetBrokerInfo();
	auto account_info = config.GetAccountInfo();
	AddAccount(account_info);
	vector<IPAddress> md_server = config.FartestCTPMDServers(5);
	AddMarketDataSource(md_server);
	setNoCloseTodayTickers(config.GetNoCloseTodayContracts());
}

void LogInHelper(TradingAccount* account) {
	if (!account->is_logged_in()) {
		try {
			account->LogInSync();
		} catch (LoginError& error) { spdlog::error(error.what()); }
	}
}
/// 登录所有注册的账号
void UnifiedTradingSystem::LogIn() {
	auto log_on_func = [](TradingAccount* account) { LogInHelper(account); };
	vector<std::jthread> thread_pool;
	thread_pool.reserve(accounts_.size());
	for (auto& [account_index, account] : accounts_) {
		thread_pool.emplace_back(log_on_func, account);
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
	if (market_data_source_ != nullptr) { market_data_source_->LogIn(); }
}
/// 登录某个注册的账号
void UnifiedTradingSystem::LogIn(const Account& account) {
	if (accounts_.contains(account)) {
		LogInHelper(accounts_[account]);
	} else {
		spdlog::error("Logging on a non-existing account({} - {})", account.first, account.second);
	}
}

/// 登出所有注册的账号
void UnifiedTradingSystem::LogOut() {
	if (market_data_source_ != nullptr) {
		market_data_source_->LogOut();
		delete market_data_source_;
		market_data_source_ = nullptr;
		market_data_ = nullptr;
	}

	for (auto& [account_index, account] : accounts_) {
		account->LogOutSync();
		delete account;
	}
	accounts_.clear();
}
/// 登出某个注册的账号
void UnifiedTradingSystem::LogOut(const Account& account) {
	if (accounts_.contains(account)) {
		accounts_[account]->LogOutSync();
		delete accounts_[account];
		accounts_.erase(account);
	} else {
		spdlog::warn("Logging off a non-existing account({} - {})", account.first, account.second);
	}
}

/// 查询合约
void UnifiedTradingSystem::QueryInstruments() {
	if (!empty()) {
		auto account = accounts_.begin();
		instrument_info_ = account->second->QueryInstruments();
	} else {
		spdlog::error("NO account registered and logged in. Cannot query market instruments!");
	}
}
void UnifiedTradingSystem::SubscribeInstruments() {
	vector<Ticker> ticker_list;
	for (auto& [ticker, instrumnet_info] : instrument_info_) { ticker_list.push_back(instrumnet_info.instrument_id); }
	market_data_source_->Subscribe(ticker_list);
}

/// 订阅指定合约
void UnifiedTradingSystem::SubscribeInstruments(const vector<Ticker>& tickers) {
	market_data_source_->Subscribe(tickers);
}

vector<Ticker> UnifiedTradingSystem::ListProducts(vector<ProductID> product_ids) {
	vector<Ticker> ticker_list;
	for (auto& product_id : product_ids) { std::ranges::transform(product_id, product_id.begin(), ::toupper); }
	for (const auto& [ticker, instrumnet_info] : instrument_info_) {
		for (const auto& product_id : product_ids) {
			if (ticker.starts_with(product_id)) {
				ticker_list.push_back(instrumnet_info.instrument_id);
				break;
			}
		}
	}
	return ticker_list;
}

/// 订阅指定品种的所有合约
void UnifiedTradingSystem::SubscribeProducts(const vector<ProductID>& product_ids) {
	auto ticker_list = ListProducts(product_ids);
	market_data_source_->Subscribe(ticker_list);
}

/// 查询所有合约的手续费
void UnifiedTradingSystem::QueryCommissionRate() {
	auto query_func = [&](TradingAccount* account) { account->QueryCommissionRate(); };
	vector<std::jthread> thread_pool;
	thread_pool.reserve(accounts_.size());
	for (auto& [account_index, account] : accounts_) { thread_pool.emplace_back(query_func, account); }
}

/// 获取所有账户的持仓
map<Account, map<InstrumentIndex, HoldingRecord>> UnifiedTradingSystem::GetHolding() const {
	map<Account, map<InstrumentIndex, HoldingRecord>> res;
	for (const auto& [account_index, account] : accounts_) {
		if (account->is_logged_in()) { res.insert({account_index, account->holding()}); }
	}
	return res;
}
/// 获取 `account` 的持仓
map<InstrumentIndex, HoldingRecord> UnifiedTradingSystem::GetHolding(const Account& account) const {
	TradingAccount* account_ptr = CheckAccount(account);
	return account_ptr->holding();
}
/// 获取所有账户的成交
map<Account, vector<TradingRecord>> UnifiedTradingSystem::GetTrades() const {
	map<Account, vector<TradingRecord>> res;
	for (const auto& [account_index, account] : accounts_) {
		if (account->is_logged_in()) { res.insert({account_index, account->trades()}); }
	}
	return res;
}
/// 获取 `account` 的成交
vector<TradingRecord> UnifiedTradingSystem::GetTrades(const Account& account) const {
	TradingAccount* account_ptr = CheckAccount(account);
	return account_ptr->trades();
}
/// 获取所有账户的委托
map<Account, vector<OrderRecord>> UnifiedTradingSystem::GetOrders() const {
	map<Account, vector<OrderRecord>> res;
	for (const auto& [account_index, account] : accounts_) {
		if (account->is_logged_in()) { res.insert({account_index, MapValues(account->orders())}); }
	}
	return res;
}
/// 获取 `account` 的委托
vector<OrderRecord> UnifiedTradingSystem::GetOrders(const Account& account) const {
	TradingAccount* account_ptr = CheckAccount(account);
	return MapValues(account_ptr->orders());
}

/**
 * @brief Json化导出数据
 * @param loc 输出文件地址
 */
void UnifiedTradingSystem::DumpInfoJson(const std::filesystem::path& loc) const {
	using nlohmann::json;
	json res;

	vector<json> account_info;
	for (auto& [account_index, account] : accounts_) { account_info.push_back(account->CurrentInfoJson()); }

	res["account_info"] = account_info;
	res["instrument_info"] = instrument_info_;
	if (market_data_source_ != nullptr) { res["market_data"] = *market_data_; }

	std::ofstream o(loc);
	o << res.dump(4) << std::endl;
	spdlog::info("Current info logged to {}", loc.string());
}

// Place order
void UnifiedTradingSystem::PlaceOrderASync(Order order) {
	Account index{order.account_name, order.broker_name};
	auto account = CheckAccount(index);
	account->PlaceOrderASync(order);
}
void UnifiedTradingSystem::PlaceOrderSync(Order order) {
	Account index{order.account_name, order.broker_name};
	auto account = CheckAccount(index);
	account->PlaceOrderSync(order);
}

/// 下灵活订单
void UnifiedTradingSystem::PlaceAdvancedOrderSync(Order order) {
	auto sub_orders = ProcessAdvancedOrder(order);
	for (auto& sub_order : sub_orders) { PlaceOrderSync(sub_order); }
}
/// ASync下灵活订单
void UnifiedTradingSystem::PlaceAdvancedOrderASync(Order order) {
	auto sub_orders = ProcessAdvancedOrder(order);
	for (auto& sub_order : sub_orders) { PlaceOrderASync(sub_order); }
}
/// 取消订单
void UnifiedTradingSystem::CancelOrder(Account account, OrderIndex index) {
	auto account_ptr = CheckAccount(account);
	account_ptr->CancelOrder(index);
}

inline bool isMultipleOfTicks(double price, double tick) {
	double div = price / tick;
	return (std::abs(std::round(div) - div) <= 0.0001);
}

/**
 * @brief 处理灵活订单
 *
 * 系统检测订单的合理性, 对于订单
 * 函数检测:
 * - 交易量 > 0
 * - 合约存在
 * - 若填写交易所, 则检测交易所合理性
 * - 剔除郑商所不支持的FOK订单
 * - 订单价格处理:
 *   -# 检查限价单的价格在涨跌停范围内, 价格为最低价格波动的整数倍.
 *   -# 将某些交易所的市价单转换成交易所支持的限价单
 *   -# 处理盘口价格偏离(列如: 卖1价 + 1tick)
 * - 若 `order.open_close` 为 `OpenCloseType::Auto`, 自动根据持仓, 手续费, 判断开平
 * - 平仓单有现有持仓.
 * @param order 订单
 * @exception OrderInfoError 订单错误
 * @return 根据现在持仓等情况生成的订单
 */
vector<Order> UnifiedTradingSystem::ProcessAdvancedOrder(Order order) {
	auto account = CheckAccount({order.account_name, order.broker_name});
	ID id = account->id();
	// volume related
	if (order.volume <= 0) {
		throw OrderInfoError(account->id(), fmt::format("Order Volume({}) for {}", order.volume, order.instrument_id));
	}

	Ticker ticker = order.instrument_id;
	std::ranges::transform(ticker, ticker.begin(), ::toupper);

	// check that instrument exist
	auto loc = instrument_info_.find(ticker);
	if (loc == instrument_info_.end()) {
		throw OrderInfoError(id, fmt::format("InstrumentID: {} does not exist", ticker));
	}
	auto& instrument_info = loc->second;
	auto& instrument_id = instrument_info.instrument_id;
	auto& exchange = instrument_info.exchange;

	order.instrument_id = instrument_id;
	order.exchange = exchange;

	// time in force check
	// 郑州不支持FOK。
	if ((exchange == Exchange::CZC) && (order.time_in_force == TimeInForce::FOK)) {
		throw OrderInfoError(id, "CZCE do not support FOK orders!");
	}

	// price related
	auto market_depth = market_data_->at(instrument_id);
	if (order.level_offset > 5 || order.level_offset < 1) {
		throw OrderInfoError(id, "Level Offset has to be between 1 and 5!");
	}
	switch (order.order_price_type) {
		case OrderPriceType::AnyPrice:
			if (exchange == Exchange::CFE) {
				order.order_price_type = OrderPriceType::FiveLevelPrice;
			} else if (exchange == Exchange::SHF) {
				order.order_price_type = OrderPriceType::LimitPrice;
				order.limit_price =
					(order.direction == Direction::Long) ? market_depth.upper_limit : market_depth.lower_limit;
			}
			break;
		case OrderPriceType::LimitPrice:
			// price has to be multiple of min_ticks
			if (!isMultipleOfTicks(order.limit_price, instrument_info.price_ticker)) {
				throw OrderInfoError(id, fmt::format(": Limit price({}) is not a multiple of price tick({})",
													 order.limit_price, instrument_info.price_ticker));
			}
			if (order.limit_price > market_depth.upper_limit) {
				throw OrderInfoError(id, fmt::format(": Limit price({}) exceeds upper limit({}))", order.limit_price,
													 market_depth.upper_limit));
			}
			if (order.limit_price < market_depth.lower_limit) {
				throw OrderInfoError(id, fmt::format(": Limit price({}) exceeds lower limit({}))", order.limit_price,
													 market_depth.lower_limit));
			}
			break;
		case OrderPriceType::BestPrice:
			if (exchange != Exchange::CFE) {
				order.order_price_type = OrderPriceType::LimitPrice;
				order.limit_price =
					(order.direction == Direction::Long) ? market_depth.ask[0].price : market_depth.bid[0].price;
			}
			break;
		case OrderPriceType::LastPrice:
			order.order_price_type = OrderPriceType::LimitPrice;
			order.limit_price = market_depth.ohlclvt.last;
			break;
		case OrderPriceType::BidPrice:
			order.order_price_type = OrderPriceType::LimitPrice;
			order.limit_price = market_depth.bid[static_cast<unsigned int>(order.level_offset - 1)].price;
			break;
		case OrderPriceType::AskPrice:
			order.order_price_type = OrderPriceType::LimitPrice;
			order.limit_price = market_depth.ask[static_cast<unsigned int>(order.level_offset - 1)].price;
			break;
		case OrderPriceType::FiveLevelPrice:
			if (exchange != Exchange::CFE) {
				order.limit_price =
					(order.direction == Direction::Long) ? market_depth.ask[4].price : market_depth.bid[4].price;
				// throw OrderInfoError(id, "Only CFFEX support FiveLevelPrice");
			}
			break;
	}
	if ((order.order_price_type != OrderPriceType::AnyPrice) &&
		(order.order_price_type != OrderPriceType::LimitPrice) &&
		(order.order_price_type != OrderPriceType::FiveLevelPrice)) {
		order.limit_price = order.limit_price + order.tick_offset * instrument_info.price_ticker *
													EnumToPositiveOrNegative(order.direction);
		order.limit_price = std::min(std::max(order.limit_price, market_depth.lower_limit), market_depth.upper_limit);
	}

	// check open_close
	auto holding = account->holding();
	auto reverse_direction = ReverseDirection(order.direction);
	auto holding_loc = holding.find({order.instrument_id, reverse_direction, order.hedge_flag});
	if ((order.open_close != OpenCloseType::Auto) && (order.open_close != OpenCloseType::Open)) {
		if (holding_loc == holding.end()) {
			throw OrderInfoError(id, "Cannot close non-existing position on " + ticker);
		}
	}

	vector<Order> order_cache;
	// 上期所、能源中心两个交易所系统都是上期所的交易系统，都有平今平昨指令，其它交易所均只有开仓、平仓指令。
	bool close_yesterday_exchange = (exchange == Exchange::SHF) || (exchange == Exchange::INE);
	if (!close_yesterday_exchange && (order.open_close == OpenCloseType::CloseYesterday)) {
		order.open_close = OpenCloseType::Close;
	}
	switch (order.open_close) {
		case OpenCloseType::Open: order_cache.push_back(order); break;
		case OpenCloseType::Close: {
			if (order.volume > holding_loc->second.total_quantity) {
				string msg = fmt::format("Closing volume {} is bigger than existing position({}) on {}", order.volume,
										 holding_loc->second.total_quantity, order.instrument_id);
				throw OrderInfoError(id, msg);
			}
			order_cache.push_back(order);
			break;
		}
		case OpenCloseType::CloseYesterday: {
			if (order.volume > holding_loc->second.pre_quantity) {
				string msg = fmt::format("Closing volume {} is bigger than existing yesterday position({}) on {}",
										 order.volume, holding_loc->second.pre_quantity, order.instrument_id);
				throw OrderInfoError(id, msg);
			}
			order_cache.push_back(order);
			break;
		}
		case OpenCloseType::CloseToday: {
			if (order.volume > holding_loc->second.today_quantity) {
				string msg = fmt::format("Closing volume {} is bigger than existing today position({}) on {}",
										 order.volume, holding_loc->second.total_quantity, order.instrument_id);
				throw OrderInfoError(id, msg);
			}
			order_cache.push_back(order);
			break;
		}
		case OpenCloseType::Auto: {
			// no reverse position, open new position directly
			if (holding_loc == holding.end()) {
				order.open_close = OpenCloseType::Open;
				order_cache.push_back(order);
				break;
			}

			HoldingRecord& holding_rec = holding_loc->second;

			// optimize closing positions
			Volume volume_left = order.volume;
			Volume close_today_vol = 0, close_pre_vol = 0;
			if (no_close_today_tickers_.contains(order.instrument_id)) {
				// 大连，如果当日有开仓，则先平今再平昨。所以有今仓则不平仓。其他情况平昨处理
				if (!((order.exchange == Exchange::DCE) && (holding_rec.today_quantity > 0))) {
					close_pre_vol = std::max(volume_left, holding_rec.pre_quantity);
					volume_left -= close_pre_vol;
				}
			} else {
				close_today_vol = std::max(volume_left, holding_rec.today_quantity);
				volume_left -= close_today_vol;

				if (volume_left > 0) {
					close_pre_vol = std::max(volume_left, holding_rec.pre_quantity);
					volume_left -= close_pre_vol;
				}
			}

			if (close_yesterday_exchange) {
				// 区分平今平昨的交易所
				if (close_pre_vol > 0) {
					order.open_close = OpenCloseType::CloseYesterday;
					order.volume = close_pre_vol;
					order_cache.push_back(order);
				}
				if (close_today_vol > 0) {
					order.open_close = OpenCloseType::CloseToday;
					order.volume = close_today_vol;
					order_cache.push_back(order);
				}
			} else {
				// 不区分平今平昨的交易所
				Volume close_quantity = close_today_vol + close_pre_vol;
				if (close_quantity > 0) {
					order.open_close = OpenCloseType::Close;
					order.volume = close_quantity;
					order_cache.push_back(order);
				}
			}

			if (volume_left > 0) {
				order.open_close = OpenCloseType::Open;
				order.volume = volume_left;
				order_cache.push_back(order);
			}
			break;
		}
		default: throw OrderInfoError(id, fmt::format("Wrong Order Type - {}", order.open_close));
	}
	return order_cache;
}

/**
 * @brief 账户清仓
 * @param account 账户索引
 * @param time_in_force 订单时间设置
 * @param price_type 价格类型
 * @exception OrderInfoError 订单错误
 */
void UnifiedTradingSystem::ClearAllHodlings(Account account, TimeInForce time_in_force, OrderPriceType price_type) {
	auto account_ptr = CheckAccount(account);
	if (price_type == OrderPriceType::LimitPrice) {
		throw OrderInfoError(account_ptr->id(), "Batch orders cannot use single limit price.");
	}
	spdlog::info("Clearing all position in account {} - {}", account.first, account.second);
	for (const auto& [holding_index, holding_rec] : account_ptr->holding()) {
		vector<Order> orders = ReversePosition(holding_rec);
		for (auto& order : orders) {
			order.account_name = account.first;
			order.broker_name = account.second;
			order.time_in_force = time_in_force;
			order.order_price_type = price_type;
			PlaceAdvancedOrderSync(order);
		}
	}
	spdlog::info("All position in account {} - {} cleared.", account.first, account.second);
}

/// 取消某个账户的未成交订单
void UnifiedTradingSystem::CancelAllPendingOrders(Account account) {
	auto account_ptr = CheckAccount(account);
	account_ptr->CancelAllPendingOrders();
}
/// 取消所有未成订单
void UnifiedTradingSystem::CancelAllPendingOrders() {
	for (auto& [account_index, account] : accounts_) { account->CancelAllPendingOrders(); }
}

// helper function
vector<Order> UnifiedTradingSystem::ReversePosition(HoldingRecord rec) {
	Order order{
		.instrument_id = rec.instrument_id,
		.exchange = rec.exchange,
		.hedge_flag = rec.hedge_flag,
		.direction = ReverseDirection(rec.direction),
		.contingent_condition = OrderContingentCondition::Immediately,
	};
	vector<Order> ret;
	if (rec.pre_quantity != 0) {
		order.open_close = OpenCloseType::CloseYesterday;
		order.volume = rec.pre_quantity;
		ret.push_back(order);
	}
	if (rec.today_quantity != 0) {
		order.open_close = OpenCloseType::CloseToday;
		order.volume = rec.today_quantity;
		ret.push_back(order);
	}
	return ret;
}
TradingAccount* UnifiedTradingSystem::CheckAccount(const Account& account) const {
	TradingAccount* account_ptr;
	try {
		account_ptr = accounts_.at(account);
	} catch (std::out_of_range) { throw AccountNotRegisteredError(account); }
	if (!account_ptr->is_logged_in()) { throw AccountNotLogedInError(account); }
	return account_ptr;
}

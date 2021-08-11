/**
 * @file data_struct.h
 * @details 常用数据类型
 */

#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>

// aliases
using ID = std::string;								 ///< 账户ID
using UserName = std::string;						 ///< 用户名
using AccountName = std::string;					 ///< 账户名
using AccountNumberStr = std::string;				 ///< 账户名
using BrokerName = std::string;						 ///< 经纪商名称
using BrokerID = std::string;						 ///< 经纪商代码
using Account = std::pair<AccountName, BrokerName>;	 ///< 账号索引
using Password = std::string;						 ///< 账户密码
using UserProductInfo = std::string;				 ///< UserProductInfo
using AppID = std::string;							 ///< AppID
using AuthCode = std::string;						 ///< AuthID

using ExchangeID = std::string;					 ///< 交易所简称
using ProductID = std::string;					 ///< 产品品种
using Ticker = std::string;						 ///< 合约代码
using IndexFutureTicker = Ticker;				 ///< 股指期货合约代码
using OptionTicker = Ticker;					 ///< 期权合约代码
using CallOptionTicker = Ticker;				 ///< 看涨期权合约代码
using PutOptionTicker = Ticker;					 ///< 看跌期权代码
using Contract = std::pair<Ticker, ExchangeID>;	 ///< 合约索引

using DateTimeStr = std::string;  ///< 日期时间字符串(YYYY-MM-DD hh:mm:ss.mmm)
using DateStr = std::string;	  ///< 日期字符串(YYYY-MM-DD)
using TimeStr = std::string;	  ///< 时间字符串(hh:mm:ss.mmm)
using YearMonth = std::string;	  ///< 年月(YYYYMM), 用于表示合约交割月份
using DaysToExpire = int;		  ///< 离到期日时长
using Money = double;			  ///< 金钱
using Price = double;			  ///< 价格
using Strike = double;			  ///< 期权行权价
using Volume = int;				  ///< 成交量
using Turnover = double;		  ///< 成交额
using Margin = double;			  ///< 保证金
using MarginRate = double;		  ///< 保证金率
using Delta = double;			  ///< 期权Delta
using PnL = double;				  ///< PnL
using ReturnRate = double;		  ///< 年化收益率
using Ratio = double;			  ///< 比例
using Offset = int;				  ///< 档位偏移

using ServerName = std::string;	  ///< 服务器名称
using IPAddress = std::string;	  ///< 服务器IP地址(start with protocal, i.e. "tcp://" or "udp://")
using FrontID = int;			  ///< 交易前置号
using SessionID = int;			  ///< Session ID
using OrderRef = long;			  ///< 委托编号
using OrderRefStr = std::string;  ///< 委托编号

using OrderIndex = std::tuple<FrontID, SessionID, OrderRef>;  ///< 委托索引

/// API类型, 现仅支持CTP
enum class APIType { CTP };

/// 连接状态,此枚举主要供程序进行识别使用
enum class ConnectionStatus {
	Uninitialized,	///< 未初始化
	Initializing,	///< 已经初始化
	Disconnected,	///< 连接已经断开，表示连接过程中遇到情况失败了
	Connecting,		///< 连接中
	Connected,		///< 连接成功
	Authorizing,	///< 授权中
	Authorized,		///< 授权成功
	Logining,		///< 登录中
	LoggedIn,		///< 登录成功
	Confirming,		///< 结算单确认中
	Confirmed,		///< 已经确认
	Done,			///< 完成，表示登录的重要过程都完成了
	LoggingOut,		///< 正在登出
	LoggedOut,		///< 已登出
};

/// 委托状态
enum class OrderStatus {
	AllTraded,				///< 全部成交
	PartTradedQueueing,		///< 部分成交还在队列中
	PartTradedNotQueueing,	///< 部分成交不在队列中
	NoTradeQueueing,		///< 未成交还在队列中
	NoTradeNotQueueing,		///< 未成交不在队列中
	RejectedByServer,		///< 柜台拒绝此订单
	RejectedByExchange,		///< 交易所拒绝此订单
	Canceled,				///< 撤单
	Unknown,				///< 未知
	NotTouched,				///< 尚未触发
	Touched,				///< 已触发
};

/// 方向
enum class OrderDirection {
	Buy,			///< 买入
	Sell,			///< 卖出
	LOFPurchase,	///< 申购,LOF申购
	LOFRedemption,	///< 赎回,LOF赎回
	ETFPurchase,	///< ETF申购
	ETFRedemption,	///< ETF赎回
	Merge,			///< 合并
	Split,			///< 拆分
	CBConvert,		///< 可转债转股，参考于https://en.wikipedia.org/wiki/Convertible_bond
	CBRedemption,	///< 可转债回售，参考于https://en.wikipedia.org/wiki/Convertible_bond
	Unknown,		///< 出现这个输出时，需要技术人员去查找原因修正代码
};

/// 有效时间
enum class TimeInForce {
	GFD,  ///< Good For Day
	FAK,  ///< Fill And Kill
	FOK,  ///< Fill(all) Or Kill
};

/// 时间条件
enum class TimeCondition {
	GFD,  ///< Good For Day
	IOC,  ///< Immediate Or Cancel
};

/// 开平仓
enum class OpenCloseType {
	Auto = 0,			  ///< 自动
	Open = 1,			  ///< 开仓
	Close = -1,			  ///< 平仓
	CloseToday = -2,	  ///< 平今仓
	CloseYesterday = -3,  ///< 平昨仓
	ForceClose = -4,	  ///< 强平
	ForceOff = -5,		  ///< ???
	LocalForceOff = -6,	  ///< ???
};

/// 投机套保标志
enum class HedgeFlagType {
	Speculation,  ///< 投机
	Arbitrage,	  ///< 套利
	Hedge,		  ///< 套保
	Covered,	  ///< 备兑,参考于CTPZQ
	MarketMaker,  ///< 做市商,参考于Femas
};

/// 交易所类型, 和Wind代码一致
enum class ExchangeType {
	SHF,  ///< 上期所
	DCE,  ///< 大商所
	CZC,  ///< 郑商所
	CFE,  ///< 中金所
	INE,  ///< 能源中心
	SH,	  ///< 上交所
	SZ,	  ///< 深交所
	SGE,  ///< 上海黄金交易所
	OC,	  ///< 全国中小企业股份转让系统,三板
	HK,	  ///< 港交所
};

/// 合约生命周期状态类型
enum class InstLifePhaseType {
	NotStart,	///< 未上市
	Started,	///< 上市
	Pause,		///< 停牌
	Expired,	///< 到期
	Issue,		///< 发行
	FirstList,	///< 首日上市
	UnList,		///< 退市
};

/// 交易阶段类型
enum class TradingPhaseType {
	BeforeTrading,	  ///< 开盘前
	NoTrading,		  ///< 非交易
	Continuous,		  ///< 连续交易
	AuctionOrdering,  ///< 集合竞价报单
	AuctionBalance,	  ///< 集合竞价价格平衡
	AuctionMatch,	  ///< 集合竞价撮合
	Closed,			  ///< 收盘
	Suspension,		  ///< 停牌时段
	Fuse,			  ///< 熔断时段
};

/// 持仓方向
enum class Direction {
	Long = 1,	 ///< 多
	Short = -1,	 ///< 空
};

/// 量价
struct PriceVolume {
	Price price;	///< 价格
	Volume volume;	///< 成交量
};

/// 高开低收量额
struct OHLCLVT {
	Price open;			///< 开盘价
	Price high;			///< 最高价
	Price low;			///< 最低价
	Price close;		///< 收盘价
	Price last;			///< 最新价
	Volume volume;		///< 成交量
	Turnover turnover;	///< 成交额
};

/// 市场行情
struct MarketDepth {
	Ticker instrument_id;  ///< 合约代码

	DateTimeStr update_time;  ///< 更新时间
	OHLCLVT ohlclvt;		  ///< 高开低收量额
	Price settle;			  ///< 结算价
	Volume open_interest;	  ///< 持仓量
	Price average_price;	  ///< 均价
	Price upper_limit;		  ///< 涨停价
	Price lower_limit;		  ///< 跌停价
	Delta delta;			  ///< 期权Delta

	std::array<PriceVolume, 5> bid;	 ///< 竞买
	std::array<PriceVolume, 5> ask;	 ///< 竞卖
};

/// 合约类型
enum class InstrumentType {
	Stock,		   ///< 股票
	Future,		   ///< 期货
	Option,		   ///< 期权
	FutureOption,  ///< 期货期权
	Bond,		   ///< 债券
	FX,			   ///< 外汇
	Index,		   ///< 指数
	ETF,		   ///< ETF
	MultiLeg,	   ///< 多腿合约
	Synthetic,	   ///< 合成合约
	LOF,		   ///< LOF
};

/// 期权类型
enum class OptionType {
	NotApplicable,	///< 不适用
	Put,			///< 看空期权
	Call,			///< 看涨期权
};

/// 触发条件
enum class OrderContingentCondition {
	Immediately,						  ///< 立即
	Touch,								  ///< 止损
	TouchProfit,						  ///< 止赢
	ParkedOrder,						  ///< 预埋单
	LastPriceGreaterThanReferencePrice,	  ///< 最新价大于条件价
	LastPriceGreaterEqualReferencePrice,  ///< 最新价大于等于条件价
	LastPriceLesserThanReferencePrice,	  ///< 最新价小于条件价
	LastPriceLesserEqualReferencePrice,	  ///< 最新价小于等于条件价
	AskPriceGreaterThanReferencePrice,	  ///< 卖一价大于条件价
	AskPriceGreaterEqualReferencePrice,	  ///< 卖一价大于等于条件价
	AskPriceLesserThanReferencePrice,	  ///< 卖一价小于条件价
	AskPriceLesserEqualReferencePrice,	  ///< 卖一价小于等于条件价
	BidPriceGreaterThanReferencePrice,	  ///< 买一价大于条件价
	BidPriceGreaterEqualReferencePrice,	  ///< 买一价大于等于条件价
	BidPriceLesserThanReferencePrice,	  ///< 买一价小于条件价
	BidPriceLesserEqualReferencePrice,	  ///< 买一价小于等于条件价
};

/// 价格条件
enum class OrderPriceType {
	AnyPrice,		 ///< 任意价
	LimitPrice,		 ///< 限价
	BestPrice,		 ///< 最优价
	LastPrice,		 ///< 最新价
	BidPrice,		 ///< 买价
	AskPrice,		 ///< 卖价
	FiveLevelPrice,	 ///< 五档价
};

/// 成交量条件
enum class VolumeCondition {
	AnyVolume,		///< 任何数量
	MinimumVolume,	///< 最小数量
	AllVolume,		///< 全部数量
};

/// MySQL数据库信息
struct MySQLConnectionInfo {
	IPAddress addr;
	std::string db_name;
	UserName user_name;
	Password password;
};

/// 账户信息
struct AccountInfo {
	AccountName account_name;		  ///< 账户名
	BrokerName broker_name;			  ///< 经纪商名称
	AccountNumberStr account_number;  ///< 账号
	Password password;				  ///< 密码
	bool enable = true;				  ///< 是否启用
};

/// 经纪商信息
struct BrokerInfo {
	BrokerName broker_name;					   ///< 经纪商名称
	APIType API_type = APIType::CTP;		   ///< API类型
	BrokerID broker_id;						   ///< 经纪商编码
	std::vector<IPAddress> trade_server_addr;  ///< 交易服务器地址
	UserProductInfo user_product_info;		   ///< UserProductInfo
	AppID app_id;							   ///< 认证ID
	AuthCode auth_code;						   ///< 认证码
	int query_rate_per_second = 0;			   ///< 每秒查询限制
};

/// 合约基础信息
struct InstrumentInfo {
	InstrumentType instrument_type;						 ///< 合约类型
	bool is_trading;									 ///< 是否在交易
	Ticker instrument_id;								 ///< 合约代码
	std::string instrument_name;						 ///< 合约名称
	ExchangeID exchange_id;								 ///< 交易所
	ProductID product_id;								 ///< 产品代码
	YearMonth deliver_month;							 ///< 交割月份
	Volume max_market_order_volume;						 ///< 最大市价委托数量
	Volume min_market_order_volume;						 ///< 最小市价委托数量
	Volume max_limit_order_volume;						 ///< 最大限价委托数量
	Volume min_limit_order_volume;						 ///< 最小限价委托数量
	Ratio volume_multiplier;							 ///< 合约乘数
	Price price_ticker;									 ///< 最小价格变动单位
	DateStr start_deliver_date;							 ///< 最早交割日
	DateStr expire_date;								 ///< 合约到期日
	MarginRate long_margin_ratio = 0;					 ///< 多头保证金比例
	MarginRate short_margin_ratio = 0;					 ///< 空头保证金比例
	bool use_max_margin_side_algorithm;					 ///< 是否使用大额担保保证金算法
	OptionType option_type = OptionType::NotApplicable;	 ///< 期权类型
	Price strike_price = 0.0;							 ///< 行权价
	Ticker underlying_instrument_id = "";				 ///< 期权标的
	Ratio underlying_multiple = 0;						 ///< 标的乘数
};

/// 资金状况
struct CapitalInfo {
	Money balance = 0;			   ///< 权益金
	Margin margin_used = 0;		   ///< 保证金占用
	Money available = 0;		   ///< 可用资金
	Money commission = 0;		   ///< 已付交易费用
	Money withdraw_allowance = 0;  ///< 可取金额
};

/// 合约索引
struct InstrumentIndex {
	Ticker instrument_id;	   ///< 合约代码
	Direction direction;	   ///< 持仓方向
	HedgeFlagType hedge_flag;  ///< 投机套保标识
	auto operator<=>(const InstrumentIndex& rhs) const = default;
};

/// 持仓记录
struct HoldingRecord {
	ExchangeID exchange_id;		///< 交易所简称
	Ticker instrument_id;		///< 合约代码
	Direction direction;		///< 持仓方向
	HedgeFlagType hedge_flag;	///< 投机套保标识
	Volume total_quantity = 0;	///< 总持仓量
	Volume today_quantity = 0;	///< 持今仓量
	Volume pre_quantity = 0;	///< 持昨仓量
};

/// 成交记录
struct TradingRecord {
	OrderRef order_ref;		   ///< 成交编号
	ExchangeID exchange_id;	   ///< 交易所简称
	Ticker instrument_id;	   ///< 合约代码
	OpenCloseType open_close;  ///< 开平
	Direction direction;	   ///< 交易方向
	HedgeFlagType hedge_flag;  ///< 投机套保标识
	Price price;			   ///< 成交价格
	Volume volume;			   ///< 成交数量
	DateTimeStr time;		   ///< 成交时间
};

/// 委托记录
struct OrderRecord {
	FrontID front_id;								///< 交易前置ID
	SessionID session_id;							///< Session ID
	OrderRef order_ref;								///< 委托编号
	ExchangeID exchange_id;							///< 交易所简称
	Ticker instrument_id;							///< 合约代码
	OpenCloseType open_close;						///< 开平
	Direction direction;							///< 交易方向
	HedgeFlagType hedge_flag;						///< 投机套保标识
	Volume total_volume;							///< 委托数量
	Volume traded_volume;							///< 已成交数量
	Volume remained_volume;							///< 未成交数量
	OrderPriceType order_price_type;				///< 价格类型
	Price limit_price;								///< 限价价格
	TimeCondition time_condition;					///< 时间条件
	OrderContingentCondition contingent_condition;	///< 触发条件
	Price reference_price;							///< 参考价格
	OrderStatus order_status;						///< 委托状态
	DateTimeStr time;								///< 委托时间
};

/// 委托单
struct Order {
	AccountName account_name;						///< 账户名
	BrokerName broker_name;							///< 经纪商名称
	Ticker instrument_id;							///< 合约代码
	ExchangeID exchange_id;							///< 交易所简称
	TimeInForce time_in_force;						///< 有效时间
	OpenCloseType open_close;						///< 开平
	HedgeFlagType hedge_flag;						///< 投机套保标识
	Direction direction;							///< 交易方向
	Volume volume;									///< 委托数量
	OrderPriceType order_price_type;				///< 价格类型
	Price limit_price = 0;							///< 限价价格
	Offset tick_offset = 0;							///< tick偏移
	Offset level_offset = 1;						///< 盘口价偏移
	OrderContingentCondition contingent_condition;	///< 触发条件
	Price reference_price = 0;						///< 参考价格
};

/// 交易手续费
struct InstrumentCommissionRate {
	Ticker instrument_id;
	MarginRate open_ratio_by_money = 0;			///< 开仓手续费率
	Margin open_ratio_by_volume = 0;			///< 开仓手续费
	MarginRate close_ratio_by_money = 0;		///< 平仓手续费率
	Margin close_ratio_by_volume = 0;			///< 平仓手续费
	MarginRate close_today_ratio_by_money = 0;	///< 平今手续费率
	Margin close_today_ratio_by_volume = 0;		///< 平今手续费
};

template <typename T>
requires std::is_same_v < std::underlying_type_t<T>,
int > int EnumToPositiveOrNegative(T val) {
	int mid = static_cast<int>(val);
	if (mid > 0) {
		return 1;
	} else if (mid < 0) {
		return -1;
	} else {
		return 0;
	}
}

template <typename K, typename V>
std::vector<V> MapValues(const std::map<K, V>& m) {
	std::vector<V> v(m.size());
	std::ranges::copy(m | std::views::values, v.begin());
	return v;
}

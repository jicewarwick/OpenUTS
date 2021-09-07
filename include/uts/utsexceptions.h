/**
 * @file utsexceptions.h
 * @details 系统异常类
 */

#pragma once

#include <exception>
#include <filesystem>
#include <string>
#include <string_view>

#include <uts/data_struct.h>

/// TradingSystem 错误基类
class UTSExceptions : public std::exception {
public:
	UTSExceptions() {}
	/// 错误信息提示
	virtual const char* what() const noexcept override { return msg_.c_str(); }

protected:
	/// 错误信息提示
	std::string msg_;
};

/// 误操作错误
class InvalidOperationError : public UTSExceptions {
	InvalidOperationError(const std::string& msg) { msg_ = msg; }
};

/// 交易账户错误基类
class TradingAccountExceptions : public UTSExceptions {
public:
	TradingAccountExceptions() {}
};

/// 网络连接错误
class NetworkError : public TradingAccountExceptions {
public:
	NetworkError(const AccountName& account_name) {
		msg_ = account_name + ": Unable to connect to server, check your network connection and server address!";
	}
};

/// 登录错误
class LoginError : public TradingAccountExceptions {
public:
	LoginError() {}
};

/// 适配文件错误
class ConfigError : public UTSExceptions {
public:
	ConfigError() {}
};

/// 文件读取错误
class IOError : public ConfigError {
public:
	IOError(const std::filesystem::path& path) { msg_ = path.string() + " does not exist!"; }
};

/// Json文件格式错误
class JsonFileError : public ConfigError {
public:
	JsonFileError(const std::filesystem::path& path) {
		msg_ = path.string() + " cannot be read in as json file, please recheck format of the input file.";
	}
};

/// 数据库读取错误
class DBFileError : public ConfigError {
public:
	DBFileError(const std::string& msg) { msg_ = msg; }
};

/// 登录信息不完整错误
class IncompleteLoginInfoError : public LoginError {
public:
	IncompleteLoginInfoError() = default;
};

/// 登录信息不完整错误
class IncompleteBrokerInfoError : public IncompleteLoginInfoError {
public:
	IncompleteBrokerInfoError(const BrokerName& broker_name) {
		msg_ = "Could not find server info for " + broker_name + ".";
	}
};

/// 未知登录失败错误
class UnknownLoginError : public LoginError {
public:
	UnknownLoginError(const AccountName& account_name) {
		msg_ = account_name + ": Unknown login error, please look up the log for details.";
	}
};

/// 客户端认证错误
class AuthorizationFailureError : public LoginError {
public:
	AuthorizationFailureError(const AccountName& account_name) { msg_ = account_name + " Authorization fail!"; }
};

/// 用户名密码错误
class AccountNumberPasswordError : public LoginError {
public:
	AccountNumberPasswordError(const AccountName& account_name) {
		msg_ = account_name + ": Account number or password invalid!";
	}
};

/// 第一次登录需修改密码错误
class FirstLoginPasswordNeedChangeError : public LoginError {
public:
	FirstLoginPasswordNeedChangeError(const AccountName& account_name) {
		msg_ = account_name + ": Need to change password at first login!";
	}
};

/// 缓存文件夹无法建立错误
class FlowFolderCreationError : public LoginError {
public:
	FlowFolderCreationError(const std::string& folder_path) { msg_ = "Cannot create flow folder: " + folder_path; }
};

/// 委托文件错误
class OrderError : public TradingAccountExceptions {
public:
	OrderError() {}
};

/// 委托信息错误
class OrderInfoError : public OrderError {
public:
	OrderInfoError(const AccountName& account_name, const std::string& message) {
		msg_ = account_name + ": " + message;
	}
};

/// 委托编号无效错误
class OrderRefError : public TradingAccountExceptions {
public:
	OrderRefError(const AccountName& account_name) {
		msg_ = account_name + ": Cannot cancel order due to wrong order ref.";
	}
};

/// 不支持委托类型错误
class OrderTypeUnsupportedError : public OrderError {
	OrderTypeUnsupportedError(const AccountName& account_name, const std::string& message) {
		msg_ = account_name + ": " + message;
	}
};

/// 交易系统错误基类
class TradingSystemException : public UTSExceptions {
public:
	TradingSystemException() {}
};

/// 未注册账户错误
class AccountNotRegisteredError : public TradingSystemException {
public:
	AccountNotRegisteredError(const Account& account_index) {
		msg_ = account_index.first + " - " + account_index.second + " is not registered the system.";
	}
};

/// 账户未登陆错误
class AccountNotLogedInError : public TradingSystemException {
public:
	AccountNotLogedInError(const Account& account_index) {
		msg_ = account_index.first + " - " + account_index.second + " is not loged in.";
	}
};

/// 未知错误
class UnknownReturnDataError : public TradingSystemException {
public:
	UnknownReturnDataError() { msg_ = "server return data unrecognized."; }
};

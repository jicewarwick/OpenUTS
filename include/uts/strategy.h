#pragma once

#include <map>
#include <string>
#include <string_view>

#include <uts/ctpmarketdata.h>
#include <uts/observer.h>
#include <uts/tradingaccount.h>

// no capital management is implemented. assuming all capital in the account is used for the strategy.
class Strategy : public Observer {
public:
	Strategy(std::string_view name) : name_(name){};
	virtual ~Strategy() = default;

	enum class AccountOrderPolicy {
		Random,
		InOrder,
		FillOneAfterAnother,
	};

	virtual void AddAccount(TradingAccount* account) = 0;
	void SetMarketData(ObservableCTPMarketDataBase* md_source) { Register(md_source); }
	void Start() {}

	virtual void OnSnapshot() = 0;

	TradingAccount* NextAccount() {
		switch (policy_) {
			case AccountOrderPolicy::Random:
			case AccountOrderPolicy::InOrder:
			case AccountOrderPolicy::FillOneAfterAnother:
			default: throw std::exception();
		}
	}

private:
	std::string name_;
	AccountOrderPolicy policy_;
};

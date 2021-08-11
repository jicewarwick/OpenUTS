#pragma once

#include <mutex>
#include <set>

/// 观察者模式传递的数据
class Data {};

class Observable;

/// 观察者类
class Observer {
	friend class Observable;

public:
	Observer() {}
	virtual ~Observer();

	/// 注册被观察者
	void Register(Observable* h);
	/// 注销被观察者
	void Unregister(Observable* h);

	/// 更新通知
	virtual void Update(Data& data) = 0;

private:
	std::mutex mutex_;
	std::set<Observable*> observables_;
};

/// 可被观察类
class Observable {
	friend class Observer;

public:
	Observable() = default;
	virtual ~Observable();

	/// 注册观察者
	void RegisterObserver(Observer* o);
	/// 注销观察者
	void UnregisterObserver(Observer* o);

	/// 通知观察者
	void NotifyObservers(Data& data);

private:
	std::mutex mutex_;
	std::set<Observer*> observers_;
};

inline Observer::~Observer() {
	for (Observable* it : observables_) { it->UnregisterObserver(this); }
}

inline void Observer::Register(Observable* h) {
	std::scoped_lock _(mutex_);
	observables_.insert(h);
}

inline void Observer::Unregister(Observable* h) {
	std::scoped_lock _(mutex_);
	observables_.erase(h);
}

inline Observable::~Observable() {
	for (Observer* it : observers_) { it->Unregister(this); }
}

inline void Observable::RegisterObserver(Observer* o) {
	std::scoped_lock _(mutex_);
	observers_.insert(o);
}

inline void Observable::UnregisterObserver(Observer* o) {
	std::scoped_lock _(mutex_);
	observers_.erase(o);
}

inline void Observable::NotifyObservers(Data& data) {
	for (Observer* observer : observers_) { observer->Update(data); }
}

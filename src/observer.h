#pragma once
#include <vector>

template <typename IFace>
class Observable {
public:
	void registerObserver(IFace& obs) {
		observers.push_back(&obs);
	}

	void unregisterObserver(const IFace& obs) {
		for (int i = 0, iEnd = observers.size(); i < iEnd; ++i) {
			if (observers[i] == &obs) {
				observers[i] = observers.back();
				observers.pop_back();
				break;
			}
		}
	}

	template <typename Func, typename... Args>
	void notify(Func func, Args&&... args) {
		for (IFace* obs : observers) {
			(obs->*func)(std::forward<Args>(args)...);
		}
	}

private:
	std::vector<IFace*> observers;
};

struct ImageManagerObserver {
	virtual void onImageChange() = 0;
};

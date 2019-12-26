#ifndef GUI_POINTER_H
#define GUI_POINTER_H

#include <TMP/traits.h>
#include <cassert>
#include <thread>
#include <type_traits>

inline const std::thread::id main_thread_id = std::this_thread::get_id();

//Wrapper to enforce that an object may only be accessed by a certain thread
template <class T>
struct Thread_checker {
	auto &operator*() {
		check();
		if constexpr (TMP::is_dereferenceable_v<T>) {
			assert(raw_value);
			return *raw_value;
		} else {
			return raw_value;
		}
	}
	auto &operator*() const {
		check();
		if constexpr (TMP::is_dereferenceable_v<T>) {
			assert(raw_value);
			return *raw_value;
		} else {
			return raw_value;
		}
	}
	T &operator->() {
		check();
		return raw_value;
	}
	T &operator->() const {
		check();
		return raw_value;
	}

	void check() const {
		assert(std::this_thread::get_id() == main_thread_id);
	}

	template <class U>
	static auto operator_spaceship(const Thread_checker<U> &lhs, const Thread_checker<U> &rhs) {
		//return std::compare_3way(lhs.t, rhs.t);
		return std::less<>{}(lhs.raw_value, rhs.raw_value) ? -1 : std::less<>{}(rhs.raw_value, lhs.raw_value) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(const Thread_checker<U> &lhs, std::nullptr_t rhs) {
		//return std::compare_3way(lhs.t, rhs);
		return std::less<>{}(lhs.raw_value, rhs) ? -1 : std::less<>{}(rhs, lhs.raw_value) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(std::nullptr_t lhs, const Thread_checker<U> &rhs) {
		//return std::compare_3way(lhs, rhs.t);
		return std::less<>{}(lhs, rhs.raw_value) ? -1 : std::less<>{}(rhs.raw_value, lhs) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(const Thread_checker<U> &lhs, U *rhs) {
		//return std::compare_3way(lhs.t, rhs);
		return std::less<>{}(lhs.raw_value, rhs) ? -1 : std::less<>{}(rhs, lhs.raw_value) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(U *lhs, const Thread_checker<U> &rhs) {
		//return std::compare_3way(lhs, rhs.t);
		return std::less<>{}(lhs, rhs.raw_value) ? -1 : std::less<>{}(rhs.raw_value, lhs) ? 1 : 0;
	}

	T raw_value{};
};

template <class T>
Thread_checker(T)->Thread_checker<T>;

template <class T>
bool operator==(const Thread_checker<T> &lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(const Thread_checker<T> &lhs, std::nullptr_t rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(std::nullptr_t lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(const Thread_checker<T> &lhs, T *rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(T *lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) == 0;
}

template <class T>
bool operator!=(const Thread_checker<T> &lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(const Thread_checker<T> &lhs, std::nullptr_t rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(std::nullptr_t lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(const Thread_checker<T> &lhs, T *rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(T *lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) != 0;
}

template <class T>
bool operator<(const Thread_checker<T> &lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(const Thread_checker<T> &lhs, std::nullptr_t rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(std::nullptr_t lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(const Thread_checker<T> &lhs, T *rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(T *lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) < 0;
}

template <class T>
bool operator<=(const Thread_checker<T> &lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(const Thread_checker<T> &lhs, std::nullptr_t rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(std::nullptr_t lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(const Thread_checker<T> &lhs, T *rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(T *lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) <= 0;
}

template <class T>
bool operator>(const Thread_checker<T> &lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(const Thread_checker<T> &lhs, std::nullptr_t rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(std::nullptr_t lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(const Thread_checker<T> &lhs, T *rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(T *lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) > 0;
}

template <class T>
bool operator>=(const Thread_checker<T> &lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(const Thread_checker<T> &lhs, std::nullptr_t rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(std::nullptr_t lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(const Thread_checker<T> &lhs, T *rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(T *lhs, const Thread_checker<T> &rhs) {
	return Thread_checker<T>::operator_spaceship(lhs, rhs) >= 0;
}

#endif //GUI_POINTER_H

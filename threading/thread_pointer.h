#ifndef THREAD_POINTER_H
#define THREAD_POINTER_H

#include <thread>

template <class T>
struct Thread_pointer {
	Thread_pointer() = default;
	Thread_pointer(T *original)
		: t{original} {}
	Thread_pointer(T *original, std::thread::id ownerid)
		: t{original}
		, onwer_thread{ownerid} {}
	T &operator*() {
		check();
		return *t;
	}
	T &operator*() const {
		check();
		return *t;
	}
	T *operator->() {
		check();
		return t;
	}
	T *operator->() const {
		check();
		return t;
	}

	std::thread::id onwer_thread = std::this_thread::get_id();

	private:
	void check() const {
		if (onwer_thread != std::this_thread::get_id()) {
			throw std::runtime_error{"Dereferenced Thread_pointer in wrong thread"};
		}
	}
	T *t = nullptr;

	public:
	template <class U>
	static auto operator_spaceship(const Thread_pointer<U> &lhs, const Thread_pointer<U> &rhs) {
		//return std::compare_3way(lhs.t, rhs.t);
		return std::less<>{}(lhs.t, rhs.t) ? -1 : std::less<>{}(rhs.t, lhs.t) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(const Thread_pointer<U> &lhs, std::nullptr_t rhs) {
		//return std::compare_3way(lhs.t, rhs);
		return std::less<>{}(lhs.t, rhs) ? -1 : std::less<>{}(rhs, lhs.t) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(std::nullptr_t lhs, const Thread_pointer<U> &rhs) {
		//return std::compare_3way(lhs, rhs.t);
		return std::less<>{}(lhs, rhs.t) ? -1 : std::less<>{}(rhs.t, lhs) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(const Thread_pointer<U> &lhs, U *rhs) {
		//return std::compare_3way(lhs.t, rhs);
		return std::less<>{}(lhs.t, rhs) ? -1 : std::less<>{}(rhs, lhs.t) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(U *lhs, const Thread_pointer<U> &rhs) {
		//return std::compare_3way(lhs, rhs.t);
		return std::less<>{}(lhs, rhs.t) ? -1 : std::less<>{}(rhs.t, lhs) ? 1 : 0;
	}
};

template <class T>
bool operator==(const Thread_pointer<T> &lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(const Thread_pointer<T> &lhs, std::nullptr_t rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(std::nullptr_t lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(const Thread_pointer<T> &lhs, T *rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(T *lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) == 0;
}

template <class T>
bool operator!=(const Thread_pointer<T> &lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(const Thread_pointer<T> &lhs, std::nullptr_t rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(std::nullptr_t lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(const Thread_pointer<T> &lhs, T *rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(T *lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) != 0;
}

template <class T>
bool operator<(const Thread_pointer<T> &lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(const Thread_pointer<T> &lhs, std::nullptr_t rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(std::nullptr_t lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(const Thread_pointer<T> &lhs, T *rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(T *lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) < 0;
}

template <class T>
bool operator<=(const Thread_pointer<T> &lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(const Thread_pointer<T> &lhs, std::nullptr_t rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(std::nullptr_t lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(const Thread_pointer<T> &lhs, T *rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(T *lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) <= 0;
}

template <class T>
bool operator>(const Thread_pointer<T> &lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(const Thread_pointer<T> &lhs, std::nullptr_t rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(std::nullptr_t lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(const Thread_pointer<T> &lhs, T *rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(T *lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) > 0;
}

template <class T>
bool operator>=(const Thread_pointer<T> &lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(const Thread_pointer<T> &lhs, std::nullptr_t rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(std::nullptr_t lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(const Thread_pointer<T> &lhs, T *rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(T *lhs, const Thread_pointer<T> &rhs) {
	return Thread_pointer<T>::operator_spaceship(lhs, rhs) >= 0;
}

#endif //THREAD_POINTER_H

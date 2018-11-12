#ifndef GUI_POINTER_H
#define GUI_POINTER_H

#include <cassert>
#include <thread>

inline const std::thread::id gui_thread_id = std::this_thread::get_id();

template <class T>
struct Gui_pointer {
	Gui_pointer(T *original = nullptr)
		: t{original} {}
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

	private:
	void check() const {
		assert(std::this_thread::get_id() == gui_thread_id);
	}
	T *t = nullptr;

	public:
	template <class U>
	static auto operator_spaceship(const Gui_pointer<U> &lhs, const Gui_pointer<U> &rhs) {
		//return std::compare_3way(lhs.t, rhs.t);
		return std::less<>{}(lhs.t, rhs.t) ? -1 : std::less<>{}(rhs.t, lhs.t) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(const Gui_pointer<U> &lhs, std::nullptr_t rhs) {
		//return std::compare_3way(lhs.t, rhs);
		return std::less<>{}(lhs.t, rhs) ? -1 : std::less<>{}(rhs, lhs.t) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(std::nullptr_t lhs, const Gui_pointer<U> &rhs) {
		//return std::compare_3way(lhs, rhs.t);
		return std::less<>{}(lhs, rhs.t) ? -1 : std::less<>{}(rhs.t, lhs) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(const Gui_pointer<U> &lhs, U *rhs) {
		//return std::compare_3way(lhs.t, rhs);
		return std::less<>{}(lhs.t, rhs) ? -1 : std::less<>{}(rhs, lhs.t) ? 1 : 0;
	}
	template <class U>
	static auto operator_spaceship(U *lhs, const Gui_pointer<U> &rhs) {
		//return std::compare_3way(lhs, rhs.t);
		return std::less<>{}(lhs, rhs.t) ? -1 : std::less<>{}(rhs.t, lhs) ? 1 : 0;
	}
};

template <class T>
bool operator==(const Gui_pointer<T> &lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(const Gui_pointer<T> &lhs, std::nullptr_t rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(std::nullptr_t lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(const Gui_pointer<T> &lhs, T *rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) == 0;
}
template <class T>
bool operator==(T *lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) == 0;
}

template <class T>
bool operator!=(const Gui_pointer<T> &lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(const Gui_pointer<T> &lhs, std::nullptr_t rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(std::nullptr_t lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(const Gui_pointer<T> &lhs, T *rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) != 0;
}
template <class T>
bool operator!=(T *lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) != 0;
}

template <class T>
bool operator<(const Gui_pointer<T> &lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(const Gui_pointer<T> &lhs, std::nullptr_t rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(std::nullptr_t lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(const Gui_pointer<T> &lhs, T *rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) < 0;
}
template <class T>
bool operator<(T *lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) < 0;
}

template <class T>
bool operator<=(const Gui_pointer<T> &lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(const Gui_pointer<T> &lhs, std::nullptr_t rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(std::nullptr_t lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(const Gui_pointer<T> &lhs, T *rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) <= 0;
}
template <class T>
bool operator<=(T *lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) <= 0;
}

template <class T>
bool operator>(const Gui_pointer<T> &lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(const Gui_pointer<T> &lhs, std::nullptr_t rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(std::nullptr_t lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(const Gui_pointer<T> &lhs, T *rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) > 0;
}
template <class T>
bool operator>(T *lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) > 0;
}

template <class T>
bool operator>=(const Gui_pointer<T> &lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(const Gui_pointer<T> &lhs, std::nullptr_t rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(std::nullptr_t lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(const Gui_pointer<T> &lhs, T *rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) >= 0;
}
template <class T>
bool operator>=(T *lhs, const Gui_pointer<T> &rhs) {
	return Gui_pointer<T>::operator_spaceship(lhs, rhs) >= 0;
}

#endif //GUI_POINTER_H

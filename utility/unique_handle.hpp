#ifndef UNIQUE_HANDLE_H
#define UNIQUE_HANDLE_H

//original source: https://github.com/milleniumbug/wiertlo/blob/0435ca25494533f9c88b5b4cbf48fec5c4ad4046/include/wiertlo/unique_handle.hpp

#include <utility>

namespace Utility {
	template <typename Policy>
	class Unique_handle {
		typename Policy::Handle_type h;

		public:
		Unique_handle(const Unique_handle &) = delete;

		typename Policy::Handle_type get() const {
			return h;
		}

		typename Policy::Handle_type release() {
			typename Policy::Handle_type temp = h;
			h = Policy::get_null();
			return temp;
		}

		explicit operator bool() const {
			return !Policy::is_null(h);
		}

		bool operator!() const {
			return !static_cast<bool>(*this);
		}

		void reset(typename Policy::Handle_type new_handle) {
			typename Policy::Handle_type old_handle = h;
			h = new_handle;
			if (!Policy::is_null(old_handle)) {
				Policy::close(old_handle);
			}
		}

		void swap(Unique_handle &other) {
			std::swap(this->h, other.h);
		}

		void reset() {
			reset(Policy::get_null());
		}

		~Unique_handle() {
			reset();
		}

		Unique_handle &operator=(Unique_handle other) noexcept {
			this->swap(other);
			return *this;
		}

		Unique_handle(Unique_handle &&other) noexcept {
			this->h = other.h;
			other.h = Policy::get_null();
		}

		Unique_handle() {
			h = Policy::get_null();
		}

		Unique_handle(typename Policy::Handle_type handle) {
			h = handle;
		}
	};
} // namespace Utility

#endif
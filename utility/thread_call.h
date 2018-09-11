#ifndef THREAD_CALL_H
#define THREAD_CALL_H

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QObject>
#include <chrono>
#include <future>
#include <type_traits>
#include <ui/mainwindow.h>

namespace Utility {
	//this function executes a function fun owned by the owning thread of object
	template <class Function>
	void async_owner_thread_execute(QObject *object, Function &&function) {
		using F = typename std::decay_t<Function>;
		struct Event : public QEvent {
			F function;
			Event(F &&p_function)
				: QEvent{QEvent::None}
				, function{std::move(p_function)} {}
			Event(const F &p_function)
				: QEvent{QEvent::None}
				, function{p_function} {}
			~Event() override {
				function();
			}
		};
		QCoreApplication::postEvent(object->thread() ? object : qApp, new Event(std::forward<Function>(function)));
	}
	template <class Function>
	void async_gui_thread_execute(Function &&function) {
		auto &mw = MainWindow::get_main_window();
		async_owner_thread_execute(&mw, std::forward<Function>(function));
	}
	template <class Function, class... Args>
	auto sync_gui_thread_execute(Function &&function, Args &&... args) {
		if (MainWindow::currently_in_gui_thread()) {
			return std::invoke(std::forward<Function>(function), std::forward<Args>(args)...);
		}
		using Return_type = std::invoke_result_t<decltype(function)>;
		std::promise<Return_type> promise;
		auto future = promise.get_future();
		async_gui_thread_execute([&promise, function = std::forward<Function>(function), tuple_args = std::forward_as_tuple<Args...>(args...)]() mutable {
			if constexpr (std::is_same_v<void, std::invoke_result_t<decltype(function)>>) {
				std::apply(function, std::move(tuple_args));
				promise.set_value();
			} else {
				promise.set_value(std::apply(function, std::move(tuple_args)));
			}
		});
		return future.get();
	}
	template <class T>
	auto get_future_value_from_gui_thread(std::future<T> &&future) {
		assert(MainWindow::currently_in_gui_thread());
		while (future.wait_for(std::chrono::milliseconds{16}) == std::future_status::timeout) {
			QApplication::processEvents();
		}
		return future.get();
	}
	template <class T>
	auto get_future_value_from_gui_thread(std::future<T> &&future, const std::chrono::milliseconds timeout) {
		assert(MainWindow::currently_in_gui_thread());
		auto start = std::chrono::high_resolution_clock::now();
		while (future.wait_for(std::chrono::milliseconds{16}) == std::future_status::timeout) {
			if (std::chrono::high_resolution_clock::now() - start > timeout) {
				throw std::runtime_error{"timeout"};
			}
			QApplication::processEvents();
		}
		return future.get();
	}
	template <class T>
	auto get_future_value(std::future<T> &&future) {
		if (MainWindow::currently_in_gui_thread()) {
			return get_future_value_from_gui_thread(std::move(future));
		}
		return future.get();
	}
	template <class T>
	auto get_future_value(std::future<T> &&future, const std::chrono::milliseconds timeout) {
		if (MainWindow::currently_in_gui_thread()) {
			return get_future_value_from_gui_thread(std::move(future), timeout);
		}
		if (future.wait_for(timeout) == std::future_status::timeout) {
			throw std::runtime_error{"timeout"};
		}
		return future.get();
	}

	template <class T>
	struct Thread_caller {
		Thread_caller(T *qt_object)
			: t{qt_object} {
			static_assert(std::is_base_of_v<QObject, T>, "Threadcaller template parameter must be derived from QObject");
		}
		template <class Function>
		void async_gui_thread_execute(Function &&function) {
			async_owner_thread_execute(t, [f = std::forward<Function>(function), t = this->t]() mutable { f(t); });
		}

		private:
		T *t;
	};
} // namespace Utility

#endif // THREAD_CALL_H
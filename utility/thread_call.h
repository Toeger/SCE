#ifndef THREAD_CALL_H
#define THREAD_CALL_H

#include <QCoreApplication>
#include <QEvent>
#include <QObject>
#include <future>
#include <type_traits>
#include <ui/mainwindow.h>

namespace Utility {
	//this function executes a function fun owned by the owning thread of object
	template <class Function>
	void async_thread_call(QObject *object, Function &&function) {
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
	void async_gui_call(Function &&function) {
		const auto mw = MainWindow::get_main_window();
		assert(mw);
		async_thread_call(mw, std::forward<Function>(function));
	}
	template <class Function, class... Args>
	auto gui_call(Function &&function, Args &&... args) {
		const auto mw = MainWindow::get_main_window();
		assert(mw);
		using Return_type = decltype(std::invoke(function, std::forward<Args>(args)...));
		std::promise<Return_type> promise;
		auto future = promise.get_future();
		async_thread_call(mw, [&promise, function = std::forward<Function>(function), tuple_args = std::forward_as_tuple<Args...>(args...)] {
			if constexpr (std::is_same_v<void, decltype(std::apply(function, std::move(tuple_args)))>) {
				std::apply(function, std::move(tuple_args));
				promise.set_value();
			} else {
				promise.set_value(std::apply(function, std::move(tuple_args)));
			}
		});
		return future.get();
	}
} // namespace Utility

#endif // THREAD_CALL_H
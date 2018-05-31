#ifndef THREAD_CALL_H
#define THREAD_CALL_H

#include <QCoreApplication>
#include <QEvent>
#include <QObject>
#include <type_traits>

#include <ui/mainwindow.h>

namespace Utility {
	//this function executes a function fun owned by the owning thread of object
	template <class Function>
	void thread_call(QObject *object, Function &&function) {
		using F = typename std::decay_t<Function>;
		struct Event : public QEvent {
			F function;
			Event(F &&p_function)
				: QEvent{QEvent::None}
				, function{std::move(p_function)} {}
			Event(const F &p_function)
				: QEvent{QEvent::None}
				, function{p_function} {}
			~Event() {
				function();
			}
		};
		QCoreApplication::postEvent(object->thread() ? object : qApp, new Event(std::forward<Function>(function)));
	}
	template <class Function>
	void gui_call(Function &&function) {
		thread_call(MainWindow::get_main_window(), std::forward<Function>(function));
	}
} // namespace Utility

#endif // THREAD_CALL_H
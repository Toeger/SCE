#include "test.h"
#include "threading/thread_pointer.h"

#include <future>

TEST_CASE("Testing Thread_pointer", "[thread-pointer]") {
	WHEN("Default-constructing") {
		Thread_pointer<int> tp;
		REQUIRE(tp == nullptr);
		REQUIRE(nullptr == tp);
		REQUIRE(tp <= nullptr);
		REQUIRE(nullptr <= tp);
		REQUIRE(tp >= nullptr);
		REQUIRE(nullptr >= tp);
		REQUIRE_FALSE(tp != nullptr);
		REQUIRE_FALSE(nullptr != tp);
		REQUIRE_FALSE(tp < nullptr);
		REQUIRE_FALSE(nullptr < tp);
		REQUIRE_FALSE(tp > nullptr);
		REQUIRE_FALSE(nullptr > tp);
	}
	WHEN("Constructing with pointer value") {
		int i;
		Thread_pointer tp = &i;
		REQUIRE(tp == &i);
		REQUIRE(&i == tp);
		REQUIRE(tp <= &i);
		REQUIRE(&i <= tp);
		REQUIRE(tp >= &i);
		REQUIRE(&i >= tp);
		REQUIRE_FALSE(tp != &i);
		REQUIRE_FALSE(&i != tp);
		REQUIRE_FALSE(tp < &i);
		REQUIRE_FALSE(&i < tp);
		REQUIRE_FALSE(tp > &i);
		REQUIRE_FALSE(&i > tp);
	}
	WHEN("Copying") {
		int i;
		Thread_pointer tp = &i;
		Thread_pointer tp2 = tp;
		REQUIRE(tp == tp2);
		int j;
		tp = &j;
		REQUIRE(tp != tp2);
		tp = tp2;
		REQUIRE(tp == tp2);
	}
	WHEN("Assigning value") {
		int i;
		Thread_pointer<int> tp;
		tp = &i;
		REQUIRE(tp == &i);
	}
	WHEN("Dereferencing in correct thread") {
		WHEN("Using primitive type") {
			int i{};
			Thread_pointer tp = &i;
			*tp = 42;
			REQUIRE(i == 42);
		}
		WHEN("Using class type") {
			struct P {
				void f() {}
			} p;
			Thread_pointer tp = &p;
			tp->f();
		}
	}
	WHEN("Dereferencing in wrong thread") {
		int i;
		Thread_pointer tp = &i;
		REQUIRE_THROWS(std::async(std::launch::async, [tp] { *tp = 42; }).get());
	}
	WHEN("Resetting thread") {
		int i;
		Thread_pointer tp = &i;
		REQUIRE_NOTHROW(std::async(std::launch::async, [tp]() mutable {
							tp.onwer_thread = std::this_thread::get_id();
							*tp = 42;
						}).get());
	}
}
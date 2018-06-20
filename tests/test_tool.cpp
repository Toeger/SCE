#include "logic/tool.h"
#include "test.h"

TEST_CASE("Testing tool", "[tool]") {
	Tool t1{}, t2{};
	REQUIRE(t1 == t2);
	REQUIRE(t1.to_string() == t2.to_string());
	REQUIRE(Tool::from_string(t1.to_string()) == t1);
	t1.path = "/some/path";
	REQUIRE(t1 != t2);
	REQUIRE(t1.to_string() != t2.to_string());
	REQUIRE(Tool::from_string(t1.to_string()) == t1);
}

#include "test_tool.h"
#include "logic/tool.h"
#include "test.h"

void test_tool() {
	Tool t1{}, t2{};
	assert_equal(t1, t2);
	assert_equal(t1.to_string(), t2.to_string());
	assert_equal(Tool::from_string(t1.to_string()), t1);
	t1.path = "/some/path";
	assert_not_equal(t1, t2);
	assert_not_equal(t1.to_string(), t2.to_string());
	assert_equal(Tool::from_string(t1.to_string()), t1);
}

#include "test_plugin.h"
#include "sce.pb.h"
#include "test.h"

void test_plugin() {
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	sce::proto::State s;
	s.set_state(42);
	std::string data;
	s.SerializeToString(&data);
}
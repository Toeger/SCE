#include "interop/language_server_protocol.h"
#include "logic/project.h"
#include "logic/tool.h"
#include "test.h"

#include <iostream>
#include <sstream>

TEST_CASE("Testing Language Server Protocol", "[lsp]") {
	Tool tool;
	tool.path = "clangd";
	WHEN("Creating and destroying an LSP server") {
		LSP::Client client{tool, {"file:///tmp"}};
	}
	WHEN("Trying to create an LSP server that doesn't exist") {
		tool.path = "nonexistant";
		REQUIRE_THROWS(LSP::Client{tool, {"file:///tmp"}});
	}
	WHEN("Running the LSP server in another thread") {
		std::async(std::launch::async, [&tool] { LSP::Client client{tool, {"file:///tmp"}}; });
	}
}
#include "interop/language_server_protocol.h"
#include "logic/tool.h"
#include "test.h"

#include <iostream>
#include <regex>
#include <sstream>

TEST_CASE("Testing Language Server Protocol", "[lsp]") {
	Tool tool;
	tool.use_tty_mode = false;
	WHEN("Creating and destroying an LSP server") {
		tool.path = "clangd-6.0";
		LSP::Client client{tool};
	}
	WHEN("Trying to create an LSP server that doesn't exist") {
		tool.path = "nonexistant";
		REQUIRE_THROWS(LSP::Client{tool});
	}
	WHEN("Running the LSP server in another thread") {
		tool.path = "clangd-6.0";
		std::async(std::launch::async, [&tool] { LSP::Client client{tool}; });
	}
}
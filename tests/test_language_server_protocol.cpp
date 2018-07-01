#include "interop/language_server_protocol.h"
#include "logic/tool.h"
#include "test.h"

#include <iostream>
#include <regex>
#include <sstream>

TEST_CASE("Testing Language Server Protocol", "[lsp]") {
	WHEN("Creating and destroying an LSP client") {
		Tool tool;
		tool.use_tty_mode = true;
		tool.path = "clangd-6.0";
		{ LSP::Client client{tool}; }
		INFO("Done");
	}
}
.DEFAULT_GOAL := help

CFLAGS = -std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Iinclude -I$(shell brew --prefix libuv)/include
LDFLAGS = -framework JavaScriptCore -L$(shell brew --prefix libuv)/lib -luv
SOURCES = $(shell find src -name "*.c")

## Build runtime
build:
	@mkdir -p build
	@clang $(CFLAGS) $(SOURCES) -o build/ragtime $(LDFLAGS)
	@if [ -z "$(SILENT)" ]; then echo "Built binary at: $(shell pwd)/build/ragtime"; fi

## Clean build artifacts
clean:
	rm -rf build compile_commands.json

## Show this help
help:
	@./scripts/generate_help.sh $(MAKEFILE_LIST)

## Generate compile commands for LSP
lsp:
	echo '[{"directory":"$(PWD)","command":"clang $(CFLAGS) $(SOURCES) $(LDFLAGS)","file":"src/main.c"}]' > compile_commands.json

## Run JS file (usage: make run FILE=path/to/file.js)
run: build
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make run FILE=path/to/file.js"; \
		exit 1; \
	fi
	./build/ragtime $(FILE)

## Install dependencies for macOS
setup/mac:
	@echo "Installing Xcode tools..."
	xcode-select --install 2>/dev/null || echo "Xcode tools already installed"
	@echo "Installing Homebrew..."
	@command -v brew >/dev/null 2>&1 || /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
	@echo "Installing libuv..."
	brew install libuv

## Install dependencies for Linux (Ubuntu/Debian)
setup/linux:
	sudo apt update
	sudo apt install libuv1-dev build-essential

## Run tests
test: 
	@make build SILENT=1
	@./tests/run_tests.sh

## Install git hooks
githooks:
	@lefthook install
	@echo "Git hooks installed"

.PHONY: build clean githooks help lsp run setup test

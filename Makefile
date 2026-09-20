# Makefile for UberPad
# Modern C++20 / Qt6 / KF6SyntaxHighlighting Editor with GUI & TUI support

BUILD_DIR   ?= build
BUILD_TYPE  ?= Release
PREFIX      ?= /usr/local
GENERATOR   ?= $(shell which ninja >/dev/null 2>&1 && echo "Ninja" || echo "Unix Makefiles")
JOBS        ?= $(shell nproc 2>/dev/null || echo 4)

.PHONY: all release debug run tui clean distclean install uninstall help

all: release

release:
	@mkdir -p $(BUILD_DIR)
	@if [ ! -f $(BUILD_DIR)/CMakeCache.txt ]; then \
		cmake -B $(BUILD_DIR) -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=Release; \
	fi
	@cmake --build $(BUILD_DIR) --parallel $(JOBS)
	@ln -sf $(BUILD_DIR)/uberpad uberpad
	@echo "==> Build complete: ./uberpad"

debug:
	@mkdir -p $(BUILD_DIR)
	@cmake -B $(BUILD_DIR) -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=Debug
	@cmake --build $(BUILD_DIR) --parallel $(JOBS)
	@ln -sf $(BUILD_DIR)/uberpad uberpad
	@echo "==> Debug build complete: ./uberpad"

run: all
	./uberpad

tui: all
	./uberpad --tui

clean:
	@if [ -d $(BUILD_DIR) ]; then \
		cmake --build $(BUILD_DIR) --target clean 2>/dev/null || true; \
	fi
	@rm -f uberpad
	@echo "==> Cleaned build artifacts."

distclean:
	@rm -rf $(BUILD_DIR) uberpad
	@echo "==> Removed $(BUILD_DIR) and binary symlink."

install: all
	@install -d $(DESTDIR)$(PREFIX)/bin
	@install -m 755 $(BUILD_DIR)/uberpad $(DESTDIR)$(PREFIX)/bin/uberpad
	@echo "==> Installed uberpad to $(DESTDIR)$(PREFIX)/bin/uberpad"

uninstall:
	@rm -f $(DESTDIR)$(PREFIX)/bin/uberpad
	@echo "==> Uninstalled uberpad from $(DESTDIR)$(PREFIX)/bin/uberpad"

help:
	@echo "UberPad Makefile commands:"
	@echo "  make              - Build release binary (default)"
	@echo "  make release      - Build optimized release binary"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make run          - Build and launch in GUI mode"
	@echo "  make tui          - Build and launch in TUI mode"
	@echo "  make clean        - Remove compiled build artifacts"
	@echo "  make distclean    - Remove build directory completely"
	@echo "  make install      - Install to $(PREFIX)/bin"
	@echo "  make uninstall    - Uninstall from $(PREFIX)/bin"
	@echo "  make help         - Show this help message"

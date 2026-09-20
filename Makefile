# Makefile for UberPad
# Modern C++20 / Qt6 / KF6SyntaxHighlighting Editor with GUI & TUI support

BUILD_DIR   ?= build
BUILD_TYPE  ?= Release
GENERATOR   ?= $(shell which ninja >/dev/null 2>&1 && echo "Ninja" || echo "Unix Makefiles")
JOBS        ?= $(shell nproc 2>/dev/null || echo 4)

# Installation layout. `make install` goes system-wide, `make install-user`
# re-invokes it with PREFIX pointed at the per-user XDG location.
PREFIX      ?= /usr/local
USER_PREFIX ?= $(HOME)/.local
BINDIR      ?= $(PREFIX)/bin
DATADIR     ?= $(PREFIX)/share
APPDIR       = $(DESTDIR)$(DATADIR)/applications
ICONDIR      = $(DESTDIR)$(DATADIR)/icons/hicolor
ICON_SIZES   = 16 24 32 48 64 128 256 512

.PHONY: all release debug run tui clean distclean \
        install install-user uninstall uninstall-user update-caches icons help

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

icons:
	@python3 resources/icons/generate-icons.py
	@echo "==> Regenerated resources/icons from the SVG source."

clean:
	@if [ -d $(BUILD_DIR) ]; then \
		cmake --build $(BUILD_DIR) --target clean 2>/dev/null || true; \
	fi
	@rm -f uberpad
	@echo "==> Cleaned build artifacts."

distclean:
	@rm -rf $(BUILD_DIR) uberpad
	@echo "==> Removed $(BUILD_DIR) and binary symlink."

# --- Installation -----------------------------------------------------------
# Installs the binary, the .desktop entry and the full hicolor icon set, then
# refreshes the desktop/icon caches so the entry shows up in the KDE, GNOME,
# XFCE, Cinnamon ... application menus without a re-login.

install: all
	@install -d $(DESTDIR)$(BINDIR)
	@install -m 755 $(BUILD_DIR)/uberpad $(DESTDIR)$(BINDIR)/uberpad
	@install -d $(APPDIR)
	@sed -e 's|^Exec=uberpad|Exec=$(BINDIR)/uberpad|' \
	     -e 's|^TryExec=uberpad|TryExec=$(BINDIR)/uberpad|' \
	     uberpad.desktop > $(APPDIR)/uberpad.desktop
	@chmod 644 $(APPDIR)/uberpad.desktop
	@for s in $(ICON_SIZES); do \
		install -d $(ICONDIR)/$${s}x$${s}/apps; \
		install -m 644 resources/icons/uberpad-$${s}.png \
			$(ICONDIR)/$${s}x$${s}/apps/uberpad.png; \
	done
	@install -d $(ICONDIR)/scalable/apps
	@install -m 644 resources/icons/uberpad.svg $(ICONDIR)/scalable/apps/uberpad.svg
	@$(MAKE) --no-print-directory update-caches
	@echo "==> Installed UberPad into $(PREFIX)"
	@echo "    binary  : $(DESTDIR)$(BINDIR)/uberpad"
	@echo "    launcher: $(APPDIR)/uberpad.desktop"

install-user:
	@$(MAKE) --no-print-directory install PREFIX=$(USER_PREFIX)
	@case ":$$PATH:" in \
		*":$(USER_PREFIX)/bin:"*) ;; \
		*) echo ""; \
		   echo "    NOTE: $(USER_PREFIX)/bin is not on your PATH."; \
		   echo "    The menu entry works regardless (it uses an absolute path),"; \
		   echo "    but to run 'uberpad' from a shell, add for fish:"; \
		   echo "      fish_add_path $(USER_PREFIX)/bin" ;; \
	esac

uninstall:
	@rm -f $(DESTDIR)$(BINDIR)/uberpad
	@rm -f $(APPDIR)/uberpad.desktop
	@for s in $(ICON_SIZES); do \
		rm -f $(ICONDIR)/$${s}x$${s}/apps/uberpad.png; \
	done
	@rm -f $(ICONDIR)/scalable/apps/uberpad.svg
	@$(MAKE) --no-print-directory update-caches
	@echo "==> Uninstalled UberPad from $(PREFIX)"

uninstall-user:
	@$(MAKE) --no-print-directory uninstall PREFIX=$(USER_PREFIX)

# Best-effort cache refresh; skipped for staged (DESTDIR) packaging builds and
# never fatal, since none of these tools are guaranteed to be present.
update-caches:
	@if [ -z "$(DESTDIR)" ]; then \
		command -v update-desktop-database >/dev/null 2>&1 && \
			update-desktop-database -q "$(APPDIR)" 2>/dev/null || true; \
		command -v gtk-update-icon-cache >/dev/null 2>&1 && \
			gtk-update-icon-cache -q -f -t "$(ICONDIR)" 2>/dev/null || true; \
		command -v kbuildsycoca6 >/dev/null 2>&1 && \
			kbuildsycoca6 --noincremental >/dev/null 2>&1 || true; \
		command -v update-mime-database >/dev/null 2>&1 && \
			update-mime-database "$(DESTDIR)$(DATADIR)/mime" 2>/dev/null || true; \
	fi

help:
	@echo "UberPad Makefile commands:"
	@echo "  make                - Build release binary (default)"
	@echo "  make release        - Build optimized release binary"
	@echo "  make debug          - Build with debug symbols"
	@echo "  make run            - Build and launch in GUI mode"
	@echo "  make tui            - Build and launch in TUI mode"
	@echo "  make icons          - Regenerate the icon set from the SVG source"
	@echo "  make clean          - Remove compiled build artifacts"
	@echo "  make distclean      - Remove build directory completely"
	@echo ""
	@echo "  make install-user   - Install for the current user only ($(USER_PREFIX))"
	@echo "  make uninstall-user - Remove the per-user installation"
	@echo "  sudo make install   - Install system-wide ($(PREFIX))"
	@echo "  sudo make uninstall - Remove the system-wide installation"
	@echo ""
	@echo "  Both install targets add the launcher to the KDE/GNOME/XFCE menus."
	@echo "  Override the location with PREFIX=..., e.g. 'make install PREFIX=/opt/uberpad'."
	@echo "  make help           - Show this help message"

# Makefile for blueyos-base utilities
# Follows BlueyOS coding conventions

CC = gcc
CFLAGS = -m32 -std=gnu11 -Wall -Wextra -O2 -fno-stack-protector
LDFLAGS = -Wl,-m,elf_i386 -static -no-pie

# Get version from git or use default
VERSION ?= 0.1.0
BUILD_NUM := $(shell git rev-list --count HEAD 2>/dev/null || echo 0)
FULL_VERSION = $(VERSION)+$(BUILD_NUM)

# Directories
SRC_DIR = src
BUILD_DIR = build
PAYLOAD_DIR = payload
INSTALL_DIR = $(PAYLOAD_DIR)/usr/bin

# musl-blueyos sysroot — prefer the system-wide BlueyOS sysroot when present;
# fall back to the local build/musl tree for fresh/CI environments.
BLUEYOS_SYSROOT ?= /opt/blueyos-sysroot
ifeq ($(shell [ -d $(BLUEYOS_SYSROOT) ] && echo yes),yes)
  MUSL_PREFIX ?= $(BLUEYOS_SYSROOT)
else
  MUSL_PREFIX ?= $(BUILD_DIR)/musl
endif

MUSL_INCLUDE := $(MUSL_PREFIX)/include
MUSL_LIB     := $(MUSL_PREFIX)/lib

CFLAGS  += -isystem $(MUSL_INCLUDE)
LDFLAGS += -L$(MUSL_LIB)

# Utilities to build
UTILITIES = find grep sed usermod gpasswd groupadd groupdel groupmod
BINARIES = $(addprefix $(BUILD_DIR)/,$(UTILITIES))

# Package info
PACKAGE_NAME = blueyos-base
PACKAGE_VERSION = $(FULL_VERSION)

.PHONY: all clean install package test musl musl-check

all: $(BINARIES)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(INSTALL_DIR):
	mkdir -p $(INSTALL_DIR)

# Helper: verify musl sysroot is present before trying to build
define check_musl
	@if [ ! -d "$(MUSL_INCLUDE)" ] || [ ! -f "$(MUSL_LIB)/libc.a" ]; then \
		echo ""; \
		echo "  [MUSL] musl sysroot not found under $(MUSL_PREFIX)"; \
		echo "         expected:"; \
		echo "           $(MUSL_INCLUDE)/  (headers)"; \
		echo "           $(MUSL_LIB)/libc.a  (static library)"; \
		echo ""; \
		echo "  To build musl-blueyos for i386:"; \
		echo "    make musl"; \
		echo "  Or point at an existing sysroot:"; \
		echo "    make MUSL_PREFIX=/path/to/musl-sysroot"; \
		echo ""; \
		exit 1; \
	fi
endef

musl-check:
	$(call check_musl)

musl:
	@bash tools/build-musl.sh --prefix=$(BUILD_DIR)/musl

# Build rules for each utility
$(BUILD_DIR)/find: $(SRC_DIR)/find.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/grep: $(SRC_DIR)/grep.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/sed: $(SRC_DIR)/sed.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/usermod: $(SRC_DIR)/usermod.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/gpasswd: $(SRC_DIR)/gpasswd.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/groupadd: $(SRC_DIR)/groupadd.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/groupdel: $(SRC_DIR)/groupdel.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/groupmod: $(SRC_DIR)/groupmod.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

# Install to payload directory for packaging
install: $(BINARIES) | $(INSTALL_DIR)
	install -m 755 $(BUILD_DIR)/find $(INSTALL_DIR)/find
	install -m 755 $(BUILD_DIR)/grep $(INSTALL_DIR)/grep
	install -m 755 $(BUILD_DIR)/sed $(INSTALL_DIR)/sed
	install -m 755 $(BUILD_DIR)/usermod $(INSTALL_DIR)/usermod
	install -m 755 $(BUILD_DIR)/gpasswd $(INSTALL_DIR)/gpasswd
	install -m 755 $(BUILD_DIR)/groupadd $(INSTALL_DIR)/groupadd
	install -m 755 $(BUILD_DIR)/groupdel $(INSTALL_DIR)/groupdel
	install -m 755 $(BUILD_DIR)/groupmod $(INSTALL_DIR)/groupmod
	@echo "Installed utilities to $(INSTALL_DIR)"

# Install to system sysroot (for image building)
install-sysroot: $(BINARIES)
	@if [ -z "$(SYSROOT)" ]; then \
		echo "Error: SYSROOT not set"; \
		exit 1; \
	fi
	install -d $(SYSROOT)/usr/bin
	install -m 755 $(BUILD_DIR)/find $(SYSROOT)/usr/bin/find
	install -m 755 $(BUILD_DIR)/grep $(SYSROOT)/usr/bin/grep
	install -m 755 $(BUILD_DIR)/sed $(SYSROOT)/usr/bin/sed
	install -m 755 $(BUILD_DIR)/usermod $(SYSROOT)/usr/bin/usermod
	install -m 755 $(BUILD_DIR)/gpasswd $(SYSROOT)/usr/bin/gpasswd
	install -m 755 $(BUILD_DIR)/groupadd $(SYSROOT)/usr/bin/groupadd
	install -m 755 $(BUILD_DIR)/groupdel $(SYSROOT)/usr/bin/groupdel
	install -m 755 $(BUILD_DIR)/groupmod $(SYSROOT)/usr/bin/groupmod
	@echo "Installed utilities to $(SYSROOT)/usr/bin"

# Create dimsim package
package: install
	@echo "Building package $(PACKAGE_NAME) version $(PACKAGE_VERSION)"
	@# Update manifest with current version
	@sed -i 's/"version": "[^"]*"/"version": "$(PACKAGE_VERSION)"/' meta/manifest.json
	@echo "Package ready for dimsim"

# Basic tests — requires i386-capable execution environment (or qemu-i386)
test: $(BINARIES)
	@echo "Running basic tests..."
	@$(BUILD_DIR)/find --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/grep --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/sed --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/usermod --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/gpasswd --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/groupadd --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/groupdel --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/groupmod --version | grep -q "$(VERSION)"
	@echo "Basic version tests passed"
	@# Test find
	@$(BUILD_DIR)/find $(SRC_DIR) -name "*.c" > /dev/null
	@echo "find test passed"
	@# Test grep
	@echo "test" | $(BUILD_DIR)/grep "test" > /dev/null
	@echo "grep test passed"
	@# Test sed
	@echo "hello" | $(BUILD_DIR)/sed 's/hello/world/' | grep -q "world"
	@echo "sed test passed"
	@echo "All tests passed!"

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(INSTALL_DIR)/find $(INSTALL_DIR)/grep $(INSTALL_DIR)/sed $(INSTALL_DIR)/usermod $(INSTALL_DIR)/gpasswd $(INSTALL_DIR)/groupadd $(INSTALL_DIR)/groupdel $(INSTALL_DIR)/groupmod
	@echo "Cleaned build artifacts"

# Show version info
version:
	@echo "Version: $(FULL_VERSION)"
	@echo "Build: $(BUILD_NUM)"

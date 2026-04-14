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
UTILITIES = find grep sed usermod gpasswd groupadd groupdel groupmod \
			uptime uname ps df du xargs print more less link ln \
			hostname tail head kill pgrep ls lsof date tee getent sort
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

$(BUILD_DIR)/uptime: $(SRC_DIR)/uptime.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/uname: $(SRC_DIR)/uname.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/ps: $(SRC_DIR)/ps.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/df: $(SRC_DIR)/df.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/du: $(SRC_DIR)/du.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/xargs: $(SRC_DIR)/xargs.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/print: $(SRC_DIR)/print.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/more: $(SRC_DIR)/more.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/less: $(SRC_DIR)/less.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/link: $(SRC_DIR)/link.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/ln: $(SRC_DIR)/ln.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/hostname: $(SRC_DIR)/hostname.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/tail: $(SRC_DIR)/tail.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/head: $(SRC_DIR)/head.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/kill: $(SRC_DIR)/kill.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/pgrep: $(SRC_DIR)/pgrep.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/ls: $(SRC_DIR)/ls.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/lsof: $(SRC_DIR)/lsof.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/date: $(SRC_DIR)/date.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/tee: $(SRC_DIR)/tee.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/getent: $(SRC_DIR)/getent.c | $(BUILD_DIR) musl-check
	$(CC) $(CFLAGS) -DVERSION=\"$(FULL_VERSION)\" -o $@ $< $(LDFLAGS) -lc

$(BUILD_DIR)/sort: $(SRC_DIR)/sort.c | $(BUILD_DIR) musl-check
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
	install -m 755 $(BUILD_DIR)/uptime $(INSTALL_DIR)/uptime
	install -m 755 $(BUILD_DIR)/uname $(INSTALL_DIR)/uname
	install -m 755 $(BUILD_DIR)/ps $(INSTALL_DIR)/ps
	install -m 755 $(BUILD_DIR)/df $(INSTALL_DIR)/df
	install -m 755 $(BUILD_DIR)/du $(INSTALL_DIR)/du
	install -m 755 $(BUILD_DIR)/xargs $(INSTALL_DIR)/xargs
	install -m 755 $(BUILD_DIR)/print $(INSTALL_DIR)/print
	install -m 755 $(BUILD_DIR)/more $(INSTALL_DIR)/more
	install -m 755 $(BUILD_DIR)/less $(INSTALL_DIR)/less
	install -m 755 $(BUILD_DIR)/link $(INSTALL_DIR)/link
	install -m 755 $(BUILD_DIR)/ln $(INSTALL_DIR)/ln
	install -m 755 $(BUILD_DIR)/hostname $(INSTALL_DIR)/hostname
	install -m 755 $(BUILD_DIR)/tail $(INSTALL_DIR)/tail
	install -m 755 $(BUILD_DIR)/head $(INSTALL_DIR)/head
	install -m 755 $(BUILD_DIR)/kill $(INSTALL_DIR)/kill
	install -m 755 $(BUILD_DIR)/pgrep $(INSTALL_DIR)/pgrep
	install -m 755 $(BUILD_DIR)/ls $(INSTALL_DIR)/ls
	install -m 755 $(BUILD_DIR)/lsof $(INSTALL_DIR)/lsof
	install -m 755 $(BUILD_DIR)/date $(INSTALL_DIR)/date
	install -m 755 $(BUILD_DIR)/tee $(INSTALL_DIR)/tee
	install -m 755 $(BUILD_DIR)/getent $(INSTALL_DIR)/getent
	install -m 755 $(BUILD_DIR)/sort $(INSTALL_DIR)/sort
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
	install -m 755 $(BUILD_DIR)/uptime $(SYSROOT)/usr/bin/uptime
	install -m 755 $(BUILD_DIR)/uname $(SYSROOT)/usr/bin/uname
	install -m 755 $(BUILD_DIR)/ps $(SYSROOT)/usr/bin/ps
	install -m 755 $(BUILD_DIR)/df $(SYSROOT)/usr/bin/df
	install -m 755 $(BUILD_DIR)/du $(SYSROOT)/usr/bin/du
	install -m 755 $(BUILD_DIR)/xargs $(SYSROOT)/usr/bin/xargs
	install -m 755 $(BUILD_DIR)/print $(SYSROOT)/usr/bin/print
	install -m 755 $(BUILD_DIR)/more $(SYSROOT)/usr/bin/more
	install -m 755 $(BUILD_DIR)/less $(SYSROOT)/usr/bin/less
	install -m 755 $(BUILD_DIR)/link $(SYSROOT)/usr/bin/link
	install -m 755 $(BUILD_DIR)/ln $(SYSROOT)/usr/bin/ln
	install -m 755 $(BUILD_DIR)/hostname $(SYSROOT)/usr/bin/hostname
	install -m 755 $(BUILD_DIR)/tail $(SYSROOT)/usr/bin/tail
	install -m 755 $(BUILD_DIR)/head $(SYSROOT)/usr/bin/head
	install -m 755 $(BUILD_DIR)/kill $(SYSROOT)/usr/bin/kill
	install -m 755 $(BUILD_DIR)/pgrep $(SYSROOT)/usr/bin/pgrep
	install -m 755 $(BUILD_DIR)/ls $(SYSROOT)/usr/bin/ls
	install -m 755 $(BUILD_DIR)/lsof $(SYSROOT)/usr/bin/lsof
	install -m 755 $(BUILD_DIR)/date $(SYSROOT)/usr/bin/date
	install -m 755 $(BUILD_DIR)/tee $(SYSROOT)/usr/bin/tee
	install -m 755 $(BUILD_DIR)/getent $(SYSROOT)/usr/bin/getent
	install -m 755 $(BUILD_DIR)/sort $(SYSROOT)/usr/bin/sort
	@echo "Installed utilities to $(SYSROOT)/usr/bin"

# Create dimsim package
# Stamps the computed PACKAGE_VERSION into meta/manifest.json, invokes dpkbuild
# to produce a .dpk archive, then restores the manifest to its committed state.
package: install
	@echo "Building package $(PACKAGE_NAME) version $(PACKAGE_VERSION)"
	@sed -i 's/"version": "[^"]*"/"version": "$(PACKAGE_VERSION)"/' meta/manifest.json
	dpkbuild build .
	@git checkout -- meta/manifest.json

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
	@$(BUILD_DIR)/uptime --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/uname --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/ps --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/df --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/du --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/xargs --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/print --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/more --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/less --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/link --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/ln --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/hostname --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/tail --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/head --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/kill --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/pgrep --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/ls --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/lsof --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/date --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/tee --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/getent --version | grep -q "$(VERSION)"
	@$(BUILD_DIR)/sort --version | grep -q "$(VERSION)"
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
	@# Test head/tail
	@echo -e "a\nb\nc\nd\ne" | $(BUILD_DIR)/head -n 3 | grep -q "a"
	@echo "head test passed"
	@echo -e "a\nb\nc\nd\ne" | $(BUILD_DIR)/tail -n 2 | grep -q "e"
	@echo "tail test passed"
	@# Test print/tee
	@$(BUILD_DIR)/print hello | grep -q "hello"
	@echo "print test passed"
	@echo "All tests passed!"

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(INSTALL_DIR)/find $(INSTALL_DIR)/grep $(INSTALL_DIR)/sed \
	       $(INSTALL_DIR)/usermod $(INSTALL_DIR)/gpasswd \
	       $(INSTALL_DIR)/groupadd $(INSTALL_DIR)/groupdel $(INSTALL_DIR)/groupmod \
	       $(INSTALL_DIR)/uptime $(INSTALL_DIR)/uname $(INSTALL_DIR)/ps \
	       $(INSTALL_DIR)/df $(INSTALL_DIR)/du $(INSTALL_DIR)/xargs \
	       $(INSTALL_DIR)/print $(INSTALL_DIR)/more $(INSTALL_DIR)/less \
	       $(INSTALL_DIR)/link $(INSTALL_DIR)/ln $(INSTALL_DIR)/hostname \
	       $(INSTALL_DIR)/tail $(INSTALL_DIR)/head $(INSTALL_DIR)/kill \
	       $(INSTALL_DIR)/pgrep $(INSTALL_DIR)/ls $(INSTALL_DIR)/lsof \
	       $(INSTALL_DIR)/date $(INSTALL_DIR)/tee $(INSTALL_DIR)/getent \
	       $(INSTALL_DIR)/sort
	@echo "Cleaned build artifacts"

# Show version info
version:
	@echo "Version: $(FULL_VERSION)"
	@echo "Build: $(BUILD_NUM)"

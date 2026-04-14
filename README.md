# blueyos-base

Base utilities and system files for BlueyOS.

## Contents

This package provides:

### System Configuration Files
- `/etc/passwd` - User account information
- `/etc/group` - Group information
- `/etc/issue` - Pre-login message
- `/etc/motd` - Message of the day

### Core Utilities
- **find** - Search for files in a directory hierarchy
- **grep** - Search text using patterns (supports regular expressions)
- **sed** - Stream editor for text transformations

## Building

```bash
make all        # Build all utilities
make test       # Run basic tests
make install    # Install to payload directory
make package    # Create dimsim package
```

### Building for BlueyOS Image

To install directly into a BlueyOS sysroot when building an image:

```bash
make install-sysroot SYSROOT=/path/to/sysroot
```

## Utility Usage

All utilities support `--version` to show version information and `--verbose` for detailed output. They also respect the `VERBOSE` environment variable set by the BlueyOS init system.

### find

Search for files matching criteria:

```bash
find [path...] [options]
  -name PATTERN    Find files matching pattern
  -type TYPE       Filter by type (f=file, d=directory)
  -maxdepth N      Descend at most N levels
  -v, --verbose    Verbose output
```

### grep

Search for patterns in files:

```bash
grep [OPTIONS] PATTERN [FILE...]
  -n              Show line numbers
  -c              Count matching lines only
  -v              Invert match (select non-matching lines)
  -i              Ignore case
  -E              Use extended regular expressions
  -h              Suppress filename prefix
```

### sed

Stream editor for text transformations:

```bash
sed [OPTIONS] 'SCRIPT' [FILE...]
  -n              Suppress automatic printing
  -i              Edit files in-place
  -e SCRIPT       Add script to commands

Supported commands:
  s/PATTERN/REPLACEMENT/[g]  Substitute
  d                           Delete line
  p                           Print line
```

## Package Information

- **Name**: blueyos-base
- **Architecture**: i386
- **Maintainer**: BlueyOS Maintainers
- **Homepage**: https://github.com/nzmacgeek/blueyos-base

## License

Part of the BlueyOS project.

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

#### Shell/Script Runtime
- **env** - Display or set environment variables for command execution
- **printf** - Format and print data (POSIX-compliant)
- **test** - Evaluate conditional expressions for shell scripts

#### File Operations
- **cp** - Copy files and directories
- **mv** - Move/rename files and directories
- **rm** - Remove files and directories
- **mkdir** - Create directories
- **cat** - Concatenate and display files
- **touch** - Create files or update timestamps
- **ln** - Create links between files

#### File Permissions & Status
- **chmod** - Change file permissions
- **chown** - Change file ownership
- **stat** - Display file/filesystem status

#### Text Processing & Search
- **find** - Search for files in a directory hierarchy
- **grep** - Search text using patterns (supports regular expressions)
- **sed** - Stream editor for text transformations
- **sort** - Sort lines of text input
- **diff** - Compare files line by line

#### Utilities
- **which** - Locate a command in PATH

### System Control
- **shutdown** - Bring the system down (halt, poweroff, or reboot)
- **reboot** - Reboot the system
- **halt** - Halt the system
- **poweroff** - Power off the system

### User and Group Management
- **usermod** - Modify user account and group membership
- **gpasswd** - Administer /etc/group (add/remove users from groups)
- **groupadd** - Create new groups
- **groupdel** - Delete groups
- **groupmod** - Modify group properties

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

### sort

Sort lines from stdin or files:

```bash
sort [OPTIONS] [FILE...]
  -n              Compare according to numeric value
  -r              Reverse the result of comparisons
  -u              Output only the first of equal lines
  -o FILE         Write result to FILE instead of stdout
  -v, --verbose   Verbose output
```
```

### usermod

Modify user account and group membership:

```bash
usermod [OPTIONS] USERNAME
  -aG GROUP       Add user to a group
  -rG GROUP       Remove user from a group
  -G GROUPS       Add user to multiple groups (comma-separated)
  -a, --append    Use with -G to append to existing groups
  -v, --verbose   Verbose output

Examples:
  usermod -aG sudo john       # Add john to sudo group
  usermod -rG sudo john       # Remove john from sudo group
  usermod -G sudo,users john  # Add john to sudo and users groups
```

### gpasswd

Administer /etc/group file (simplified interface):

```bash
gpasswd [OPTIONS] GROUP
  -a, --add USER      Add USER to GROUP
  -d, --delete USER   Remove USER from GROUP
  -v, --verbose       Verbose output

Examples:
  gpasswd -a john sudo     # Add john to sudo group
  gpasswd -d john sudo     # Remove john from sudo group
```

**Note**: Both `usermod` and `gpasswd` require root privileges to modify group membership.

### groupadd

Create a new group:

```bash
groupadd [OPTIONS] GROUP
  -g, --gid GID     Use GID for the new group
  -r, --system      Create a system group (GID 100-999)
  -v, --verbose     Verbose output

Examples:
  groupadd developers           # Create a new group with auto-assigned GID
  groupadd -g 1500 developers   # Create group with specific GID
  groupadd -r daemon            # Create a system group
```

### groupdel

Delete a group:

```bash
groupdel [OPTIONS] GROUP
  -v, --verbose     Verbose output

Examples:
  groupdel developers   # Delete the developers group

Note: Cannot delete a group that is the primary group of any user
```

### groupmod

Modify group properties:

```bash
groupmod [OPTIONS] GROUP
  -g, --gid GID         Change the group ID to GID
  -n, --new-name NAME   Change the group name to NAME
  -v, --verbose         Verbose output

Examples:
  groupmod -g 1600 developers        # Change GID
  groupmod -n devs developers        # Rename group
  groupmod -g 1600 -n devs developers  # Change both GID and name
```

**Note**: All group management utilities (`groupadd`, `groupdel`, `groupmod`) require root privileges.

### shutdown

Bring the system down:

```bash
shutdown [OPTIONS]
  -h, --halt       Halt the system (don't power off)
  -P, --poweroff   Power off the system (default)
  -r, --reboot     Reboot the system
  -n, --no-sync    Don't sync before shutdown
  -f, --force      Force immediate shutdown (skip sync)
  -v, --verbose    Verbose output

Examples:
  shutdown              # Power off the system (default)
  shutdown -h           # Halt the system without powering off
  shutdown -r           # Reboot the system
  shutdown -f -P        # Force immediate poweroff without sync
```

### reboot

Reboot the system:

```bash
reboot [OPTIONS]
  -f, --force      Force immediate reboot (skip sync)
  -n, --no-sync    Don't sync before reboot
  -v, --verbose    Verbose output

Examples:
  reboot               # Reboot the system
  reboot -f            # Force immediate reboot
```

### halt

Halt the system:

```bash
halt [OPTIONS]
  -p, --poweroff   Power off after halt
  -f, --force      Force immediate halt (skip sync)
  -n, --no-sync    Don't sync before halt
  -v, --verbose    Verbose output

Examples:
  halt                 # Halt the system
  halt -p              # Halt and power off
```

### poweroff

Power off the system:

```bash
poweroff [OPTIONS]
  -f, --force      Force immediate poweroff (skip sync)
  -n, --no-sync    Don't sync before poweroff
  -v, --verbose    Verbose output

Examples:
  poweroff             # Power off the system
  poweroff -f          # Force immediate poweroff
```

**Note**: All system control utilities (`shutdown`, `reboot`, `halt`, `poweroff`) require root privileges.

## Package Information

- **Name**: blueyos-base
- **Architecture**: i386
- **Maintainer**: BlueyOS Maintainers
- **Homepage**: https://github.com/nzmacgeek/blueyos-base

## License

Part of the BlueyOS project.

# Setup and Installation Guide

This guide provides detailed instructions for setting up and installing the Network Monitoring and Visualization Tool.

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Dependencies](#dependencies)
3. [Installation Steps](#installation-steps)
4. [Configuration](#configuration)
5. [Building from Source](#building-from-source)
6. [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Requirements
- **Operating System**: Linux (Ubuntu 18.04+, Debian 10+, CentOS 7+) or macOS 10.14+
- **CPU**: 1 GHz dual-core processor
- **RAM**: 512 MB
- **Disk Space**: 100 MB for installation
- **Network**: Network interface card with appropriate access to Cisco devices

### Recommended Requirements
- **Operating System**: Ubuntu 20.04 LTS or macOS 11+
- **CPU**: 2 GHz quad-core processor
- **RAM**: 2 GB
- **Disk Space**: 500 MB for installation and logs
- **Network**: Gigabit Ethernet with SNMP access to devices

## Dependencies

### Required Libraries

#### On Ubuntu/Debian:

```bash
# Update package list
sudo apt-get update

# Install build tools
sudo apt-get install build-essential gcc make git

# Install required libraries
sudo apt-get install libpcap-dev libncurses5-dev libsnmp-dev
sudo apt-get install libpthread-stubs0-dev libssl-dev

# Optional: Install debugging and analysis tools
sudo apt-get install gdb valgrind cppcheck clang-format
```

#### On CentOS/RHEL:

```bash
# Install development tools
sudo yum groupinstall "Development Tools"
sudo yum install gcc make git

# Install required libraries
sudo yum install libpcap-devel ncurses-devel net-snmp-devel
sudo yum install openssl-devel

# Optional: Install debugging tools
sudo yum install gdb valgrind
```

#### On macOS:

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install gcc make git
brew install libpcap ncurses net-snmp openssl

# Optional: Install debugging tools
brew install gdb valgrind
```

### Library Versions
- **GCC**: 7.0 or higher (C11 support required)
- **libpcap**: 1.8.0 or higher
- **ncurses**: 6.0 or higher
- **net-snmp**: 5.7 or higher

## Installation Steps

### Step 1: Clone the Repository

```bash
git clone https://github.com/Sahrinu/networks.git
cd networks
```

### Step 2: Build the Project

```bash
# Build the release version
make release

# Or build the debug version for development
make debug
```

### Step 3: Verify Installation

```bash
# Check if the binary was created
ls -l bin/netmon

# Run the application
./bin/netmon
```

### Step 4: System-wide Installation (Optional)

```bash
# Install to /usr/local/bin (requires sudo)
sudo make install

# Now you can run from anywhere
netmon
```

## Configuration

### Device Configuration

1. Edit the device configuration file:

```bash
nano configs/devices.conf
```

2. Add your Cisco devices in the following format:

```
# Format: hostname;ip_address;snmp_community;port
# Lines starting with # are comments

router1;192.168.1.1;public;161
switch1;192.168.1.2;public;161
firewall1;192.168.1.254;private;161
```

### SNMP Configuration

Ensure SNMP is enabled on your Cisco devices:

```cisco
# Enable SNMP on Cisco IOS
config terminal
snmp-server community public RO
snmp-server community private RW
end
write memory
```

### Firewall Configuration

Ensure UDP port 161 (SNMP) is open between the monitoring host and target devices:

```bash
# On Ubuntu with UFW
sudo ufw allow 161/udp

# On CentOS/RHEL with firewalld
sudo firewall-cmd --permanent --add-port=161/udp
sudo firewall-cmd --reload
```

## Building from Source

### Standard Build

```bash
# Clean previous builds
make clean

# Build with default settings
make
```

### Debug Build

For development and debugging:

```bash
make debug
```

This creates a binary with:
- Debug symbols (-g)
- No optimization (-O0)
- Debug macros enabled

### Release Build

For production deployment:

```bash
make release
```

This creates an optimized binary with:
- Optimization level 2 (-O2)
- Debug assertions disabled
- Smaller binary size

### Custom Build Options

You can override compiler flags:

```bash
# Build with additional warnings
make CFLAGS="-Wall -Wextra -Werror"

# Build with custom optimization
make CFLAGS="-O3 -march=native"
```

## Testing

### Run All Tests

```bash
make test
```

### Run Individual Tests

```bash
# Test network functions
./bin/test_network

# Test monitoring functions
./bin/test_monitoring

# Test utility functions
./bin/test_utils
```

### Memory Leak Testing

```bash
# Build debug version
make debug

# Run with valgrind
valgrind --leak-check=full --show-leak-kinds=all ./bin/netmon
```

## Troubleshooting

### Common Issues

#### Issue 1: Missing Libraries

**Symptom**: Error messages about missing headers or libraries during compilation

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install libpcap-dev libncurses5-dev libsnmp-dev

# macOS
brew install libpcap ncurses net-snmp
```

#### Issue 2: Permission Denied

**Symptom**: Cannot capture packets or access network interfaces

**Solution**:
```bash
# Grant capabilities to the binary (Linux)
sudo setcap cap_net_raw,cap_net_admin=eip ./bin/netmon

# Or run as root (not recommended for production)
sudo ./bin/netmon
```

#### Issue 3: SNMP Connection Failed

**Symptom**: Cannot connect to devices via SNMP

**Solutions**:
1. Verify SNMP is enabled on the device
2. Check community string is correct
3. Verify network connectivity:
   ```bash
   ping 192.168.1.1
   snmpwalk -v2c -c public 192.168.1.1
   ```
4. Check firewall rules allow UDP port 161

#### Issue 4: Compilation Errors

**Symptom**: Errors during `make` command

**Solution**:
```bash
# Clean and rebuild
make clean
make

# Check GCC version (must be 7.0+)
gcc --version

# Update if necessary
sudo apt-get install gcc-9
```

#### Issue 5: Segmentation Fault

**Symptom**: Program crashes with segmentation fault

**Solution**:
```bash
# Build debug version
make debug

# Run with GDB
gdb ./bin/netmon
(gdb) run
# When it crashes:
(gdb) backtrace
```

### Getting Help

If you encounter issues not covered here:

1. Check the [GitHub Issues](https://github.com/Sahrinu/networks/issues)
2. Review the [Architecture Documentation](ARCHITECTURE.md)
3. Enable verbose logging (if available)
4. Run with debugging tools (gdb, valgrind)

### Logging

Enable debug logging for troubleshooting:

```bash
# Set environment variable
export NETMON_DEBUG=1
./bin/netmon
```

### Uninstallation

To remove the system-wide installation:

```bash
sudo make uninstall
```

To remove all build artifacts:

```bash
make clean
```

## Next Steps

After successful installation:

1. Review the [Architecture Documentation](ARCHITECTURE.md)
2. Check [Usage Examples](../examples/usage.md)
3. Configure your devices in `configs/devices.conf`
4. Start monitoring!

## Support

For additional support:
- Open an issue on GitHub
- Consult the main [README.md](../README.md)
- Review example configurations in `examples/`

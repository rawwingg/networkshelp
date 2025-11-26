# Network Monitoring and Visualization Tool

A comprehensive network monitoring and visualization tool built with C-Language for Cisco Networking Devices. This project provides real-time network monitoring, data collection, and visualization capabilities for network administrators and engineers.

## Project Overview

This tool enables network administrators to:
- Monitor Cisco network devices in real-time
- Collect and analyze network performance metrics
- Visualize network topology and statistics
- Generate alerts for network anomalies
- Track bandwidth utilization and device health

## Objectives

1. **Real-time Monitoring**: Continuously monitor network devices and collect performance data
2. **Data Visualization**: Present network statistics in an intuitive, visual format
3. **Alerting System**: Notify administrators of critical network events
4. **Historical Analysis**: Store and analyze historical network data
5. **Multi-device Support**: Monitor multiple Cisco devices simultaneously

## Technologies and Tools Required

### Development Tools
- **C Compiler**: GCC (GNU Compiler Collection) 7.0 or higher
- **Make**: GNU Make 4.0 or higher
- **Git**: Version control system
- **GDB**: GNU Debugger (for debugging)

### Libraries
- **libpcap**: Packet capture library
- **ncurses**: Terminal UI library for visualization
- **pthread**: POSIX threads for concurrent operations
- **libsnmp**: SNMP library for device communication

### Cisco Networking
- **SNMP**: Simple Network Management Protocol support
- **SSH**: Secure Shell for device access
- **Telnet**: Alternative device access (less secure)
- **Cisco IOS**: Compatible with Cisco IOS devices

### Operating System
- Linux (Ubuntu 20.04+ or similar)
- macOS (with Homebrew for dependencies)

## Project Structure

```
.
├── README.md              # Project documentation
├── Makefile              # Build system
├── .gitignore            # Git ignore rules
├── src/                  # Source code
│   ├── main.c           # Main program entry point
│   ├── network/         # Network communication modules
│   ├── monitoring/      # Monitoring and data collection
│   ├── visualization/   # UI and visualization components
│   └── utils/           # Utility functions
├── include/              # Header files
│   └── netmon.h         # Main header file
├── tests/               # Unit and integration tests
├── docs/                # Documentation
│   ├── SETUP.md         # Setup and installation guide
│   └── ARCHITECTURE.md  # System architecture
├── configs/             # Configuration files
│   └── devices.conf     # Device configuration
└── examples/            # Example configurations
    └── usage.md         # Usage examples
```

## Expected Outcomes

Upon completion, this project will deliver:

1. **Functional Network Monitor**: A working application capable of monitoring Cisco devices
2. **Real-time Visualization**: Console-based interface displaying network statistics
3. **Data Collection System**: Automated collection of network metrics
4. **Alert Mechanism**: Configurable alerting for network events
5. **Documentation**: Complete technical and user documentation
6. **Test Suite**: Comprehensive test coverage for core functionality

## Getting Started

### Prerequisites

Install required dependencies on Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install build-essential gcc make git
sudo apt-get install libpcap-dev libncurses5-dev libsnmp-dev
sudo apt-get install libpthread-stubs0-dev
```

On macOS with Homebrew:

```bash
brew install gcc make git
brew install libpcap ncurses net-snmp
```

### Installation

1. Clone the repository:
```bash
git clone https://github.com/Sahrinu/networks.git
cd networks
```

2. Build the project:
```bash
make
```

3. Run the application:
```bash
./bin/netmon
```

### Quick Start

1. Configure your devices in `configs/devices.conf`
2. Start the monitoring tool: `./bin/netmon`
3. Select monitoring options from the menu
4. View real-time statistics and logs

## Features

### Current Features
- [x] Project structure and build system
- [x] Basic menu system

### Development Roadmap

#### Phase 1: Foundation (Weeks 1-2)
- [ ] Network device discovery
- [ ] SNMP communication module
- [ ] Basic data collection

#### Phase 2: Monitoring (Weeks 3-4)
- [ ] Real-time performance monitoring
- [ ] Bandwidth utilization tracking
- [ ] Device health checks
- [ ] Log collection and parsing

#### Phase 3: Visualization (Weeks 5-6)
- [ ] Console-based dashboard
- [ ] Network topology display
- [ ] Real-time graph rendering
- [ ] Statistical analysis views

#### Phase 4: Advanced Features (Weeks 7-8)
- [ ] Alert system implementation
- [ ] Historical data storage
- [ ] Configuration backup/restore
- [ ] Multi-threaded monitoring

#### Phase 5: Testing & Documentation (Week 9)
- [ ] Unit test suite
- [ ] Integration testing
- [ ] Performance optimization
- [ ] Complete documentation

## Building the Project

### Build Targets

```bash
make           # Build the project (default target)
make debug     # Build with debug symbols
make release   # Build optimized release version
make test      # Run tests
make clean     # Remove build artifacts
make install   # Install to system
make uninstall # Remove from system
```

### Development Build

For active development with debugging:

```bash
make debug
gdb ./bin/netmon
```

## Configuration

Edit `configs/devices.conf` to add your Cisco devices:

```
# Device Configuration Format
# hostname;ip_address;snmp_community;port
router1;192.168.1.1;public;161
switch1;192.168.1.2;public;161
```

## Testing

Run the test suite:

```bash
make test
```

Individual test categories:

```bash
./tests/test_network
./tests/test_monitoring
./tests/test_utils
```

## Documentation

Detailed documentation is available in the `docs/` directory:

- [Setup Guide](docs/SETUP.md) - Installation and configuration
- [Architecture](docs/ARCHITECTURE.md) - System design and components
- [Usage Examples](examples/usage.md) - Common use cases and tutorials

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/new-feature`)
3. Commit your changes (`git commit -am 'Add new feature'`)
4. Push to the branch (`git push origin feature/new-feature`)
5. Create a Pull Request

## License

This project is developed as part of a course project for educational purposes.

## Support

For issues, questions, or contributions, please open an issue on GitHub.

## Authors

- Network Monitoring Team

## Acknowledgments

- Cisco Systems for networking protocols and documentation
- Open source community for libraries and tools
- Course instructors and peers for guidance and support
# System Architecture Documentation

This document describes the architecture and design of the Network Monitoring and Visualization Tool.

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Module Design](#module-design)
4. [Data Flow](#data-flow)
5. [Key Components](#key-components)
6. [Threading Model](#threading-model)
7. [Communication Protocols](#communication-protocols)
8. [Data Structures](#data-structures)

## Overview

The Network Monitoring and Visualization Tool is designed as a modular, multi-threaded C application for monitoring Cisco networking devices. The system follows a layered architecture pattern with clear separation of concerns.

### Design Goals

1. **Modularity**: Components are loosely coupled and independently testable
2. **Scalability**: Support for monitoring hundreds of devices simultaneously
3. **Performance**: Efficient resource usage with minimal overhead
4. **Reliability**: Robust error handling and recovery mechanisms
5. **Maintainability**: Clean code structure following C best practices

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────┐
│                  User Interface Layer               │
│            (Console Menu & Visualization)           │
└─────────────────┬───────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────┐
│              Application Logic Layer                │
│         (Monitoring, Statistics, Alerts)            │
└─────────────────┬───────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────┐
│           Network Communication Layer               │
│              (SNMP, Ping, SSH/Telnet)               │
└─────────────────┬───────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────┐
│                 Data Storage Layer                  │
│         (Device Info, Stats, Configuration)         │
└─────────────────────────────────────────────────────┘
```

### Layer Responsibilities

#### 1. User Interface Layer
- **Location**: `src/visualization/`
- **Purpose**: Handle user interaction and display
- **Components**:
  - Menu system
  - Console-based dashboard
  - Graph rendering
  - Alert display

#### 2. Application Logic Layer
- **Location**: `src/monitoring/`
- **Purpose**: Core business logic
- **Components**:
  - Device polling
  - Data collection
  - Statistics calculation
  - Alert generation

#### 3. Network Communication Layer
- **Location**: `src/network/`
- **Purpose**: Device communication
- **Components**:
  - SNMP client
  - Ping implementation
  - SSH/Telnet client
  - Protocol handlers

#### 4. Data Storage Layer
- **Location**: `src/utils/`
- **Purpose**: Data management
- **Components**:
  - Device database
  - Configuration parser
  - Statistics storage
  - Log management

## Module Design

### Main Module (`src/main.c`)

Entry point and main control loop.

**Responsibilities**:
- Initialize system components
- Display main menu
- Handle user input
- Coordinate module interactions
- Clean shutdown

**Key Functions**:
```c
int main(int argc, char *argv[]);
static void display_menu(void);
static void handle_menu_choice(int choice);
```

### Network Module (`src/network/`)

Handles all network communication with devices.

**Files**:
- `snmp_client.c` - SNMP operations
- `ping.c` - ICMP ping implementation
- `ssh_client.c` - SSH communication
- `protocol.c` - Protocol utilities

**Key Functions**:
```c
int snmp_get(const char *ip, const char *community, const char *oid, char *result, size_t result_size);
int snmp_walk(const char *ip, const char *community, const char *oid);
int ping_device(const char *ip, int *response_time);
```

### Monitoring Module (`src/monitoring/`)

Implements device monitoring and data collection.

**Files**:
- `device_monitor.c` - Device polling
- `data_collector.c` - Metrics collection
- `statistics.c` - Statistics calculation
- `alert_manager.c` - Alert handling

**Key Functions**:
```c
int start_monitoring(void);
int poll_device(const char *hostname);
int get_statistics(network_stats_t *stats);
int add_alert(const alert_t *alert);
```

### Visualization Module (`src/visualization/`)

Handles display and user interface.

**Files**:
- `display.c` - Screen management
- `dashboard.c` - Main dashboard
- `graphs.c` - Graph rendering
- `topology.c` - Topology visualization

**Key Functions**:
```c
int init_display(void);
void update_display(void);
void draw_statistics(const network_stats_t *stats);
void draw_topology(void);
```

### Utilities Module (`src/utils/`)

Common utilities and helper functions.

**Files**:
- `config.c` - Configuration file handling
- `logger.c` - Logging system
- `device_db.c` - Device database
- `helpers.c` - General utilities

**Key Functions**:
```c
int load_config(const char *filename);
int add_device(const network_device_t *device);
const char* status_to_string(device_status_t status);
```

## Data Flow

### Device Monitoring Flow

```
1. User starts monitoring
         ↓
2. Load device config (configs/devices.conf)
         ↓
3. Initialize device list
         ↓
4. Create monitoring threads
         ↓
5. For each device:
   - Send SNMP request
   - Parse response
   - Update device status
   - Calculate statistics
   - Check thresholds
   - Generate alerts if needed
         ↓
6. Update display
         ↓
7. Wait for poll interval
         ↓
8. Repeat from step 5
```

### SNMP Query Flow

```
Application Layer
    ↓ snmp_get()
Network Module
    ↓ Build SNMP packet
    ↓ Send UDP packet
Network Stack
    ↓
Cisco Device (SNMP Agent)
    ↓ Process request
    ↓ Send response
Network Stack
    ↓
Network Module
    ↓ Parse response
    ↓ Extract data
Application Layer
    ↓ Update device info
```

## Key Components

### Device Manager

**Purpose**: Manage the list of monitored devices

**Operations**:
- Add/remove devices
- Update device status
- Query device information
- Persist device configuration

### Polling Engine

**Purpose**: Periodically poll devices for metrics

**Features**:
- Configurable poll intervals
- Parallel polling with thread pool
- Automatic retry on failure
- Timeout handling

### Statistics Engine

**Purpose**: Calculate and store network statistics

**Metrics**:
- Device availability
- Response time
- Bandwidth utilization
- Error rates
- CPU and memory usage

### Alert Manager

**Purpose**: Generate and manage alerts

**Alert Types**:
- Device down
- High CPU usage
- High memory usage
- High error rate
- Threshold violations

## Threading Model

### Thread Architecture

```
Main Thread
    │
    ├─> Monitoring Thread
    │   ├─> Device Poll Thread 1
    │   ├─> Device Poll Thread 2
    │   └─> Device Poll Thread N
    │
    ├─> Statistics Thread
    │   └─> Periodic statistics calculation
    │
    └─> Display Thread
        └─> Update UI periodically
```

### Synchronization

**Mutexes**:
- Device list mutex
- Statistics mutex
- Alert queue mutex
- Configuration mutex

**Condition Variables**:
- Poll interval condition
- Alert notification condition

### Thread Safety

- All shared data structures protected by mutexes
- Read-write locks for frequently read data
- Lock-free data structures where appropriate
- Careful lock ordering to prevent deadlocks

## Communication Protocols

### SNMP (Simple Network Management Protocol)

**Version**: SNMPv2c (with v3 support planned)

**Operations**:
- GET - Retrieve single OID value
- GETNEXT - Retrieve next OID in tree
- WALK - Retrieve all OIDs under subtree

**Common OIDs**:
- System description: `.1.3.6.1.2.1.1.1.0`
- Uptime: `.1.3.6.1.2.1.1.3.0`
- Interfaces: `.1.3.6.1.2.1.2.2.1`
- CPU: `.1.3.6.1.4.1.9.2.1.56.0` (Cisco)

### ICMP (Ping)

**Purpose**: Basic connectivity testing

**Implementation**:
- Raw socket creation
- ICMP Echo Request
- Response time measurement
- Packet loss detection

### SSH (Future Enhancement)

**Purpose**: Secure device access for configuration

**Planned Features**:
- Execute CLI commands
- Retrieve configuration
- Backup/restore operations

## Data Structures

### Network Device Structure

```c
typedef struct {
    char hostname[MAX_HOSTNAME_LEN];
    char ip_address[MAX_IP_LEN];
    char snmp_community[MAX_COMMUNITY_LEN];
    uint16_t port;
    device_status_t status;
    time_t last_seen;
    uint64_t bytes_in;
    uint64_t bytes_out;
    uint32_t errors_in;
    uint32_t errors_out;
    float cpu_usage;
    float memory_usage;
    int response_time_ms;
} network_device_t;
```

### Statistics Structure

```c
typedef struct {
    uint32_t total_devices;
    uint32_t active_devices;
    uint32_t inactive_devices;
    uint64_t total_bytes_in;
    uint64_t total_bytes_out;
    uint32_t total_alerts;
    float avg_response_time;
} network_stats_t;
```

### Alert Structure

```c
typedef struct {
    time_t timestamp;
    alert_severity_t severity;
    char device_hostname[MAX_HOSTNAME_LEN];
    char message[256];
} alert_t;
```

## Performance Considerations

### Memory Management

- Pre-allocated buffers for network packets
- Memory pools for frequently allocated structures
- Careful cleanup to prevent leaks
- Valgrind testing for memory safety

### Network Optimization

- Connection pooling where applicable
- Batch SNMP requests
- Adaptive poll intervals
- Efficient packet parsing

### CPU Optimization

- Thread pool to limit thread creation overhead
- Lock-free algorithms where possible
- Efficient data structures (hash tables, etc.)
- Minimal blocking operations

## Error Handling

### Strategy

1. **Detection**: Check return codes and validate data
2. **Logging**: Log all errors with context
3. **Recovery**: Attempt automatic recovery
4. **Notification**: Alert user when necessary
5. **Graceful Degradation**: Continue operation if possible

### Error Categories

- **Network Errors**: Timeouts, connection refused, unreachable
- **Protocol Errors**: Invalid SNMP responses, malformed packets
- **System Errors**: Memory allocation, file I/O failures
- **Configuration Errors**: Invalid device config, missing files

## Security Considerations

### Current Implementation

- SNMP community strings in configuration (v2c)
- No encryption for SNMP communication
- File permissions for configuration files

### Planned Enhancements

- SNMPv3 with authentication and encryption
- Secure storage of credentials
- TLS for future web interface
- Role-based access control

## Future Enhancements

1. **Web Interface**: HTML5 dashboard
2. **Database Backend**: Store historical data
3. **Distributed Monitoring**: Multiple monitoring nodes
4. **Machine Learning**: Anomaly detection
5. **REST API**: Integration with other systems
6. **NETCONF/YANG**: Modern device configuration
7. **Flow Analysis**: NetFlow/sFlow support

## References

- RFC 1157 - SNMP v1
- RFC 3416 - SNMP v2c
- RFC 792 - ICMP
- Cisco SNMP Object Navigator
- Net-SNMP Documentation

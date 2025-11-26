# Usage Examples and Tutorials

This document provides practical examples and tutorials for using the Network Monitoring and Visualization Tool.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Basic Usage](#basic-usage)
3. [Configuration Examples](#configuration-examples)
4. [Common Scenarios](#common-scenarios)
5. [Advanced Usage](#advanced-usage)
6. [Troubleshooting Tips](#troubleshooting-tips)

## Getting Started

### First Time Setup

1. **Install the application**:
   ```bash
   cd /home/runner/work/networks/networks
   make release
   ```

2. **Configure your first device**:
   ```bash
   nano configs/devices.conf
   ```
   
   Add a line for your device:
   ```
   my-router;192.168.1.1;public;161
   ```

3. **Run the application**:
   ```bash
   ./bin/netmon
   ```

### Quick Test with Sample Device

If you have a Cisco device or simulator (GNS3, Packet Tracer):

```bash
# Test SNMP connectivity first
snmpwalk -v2c -c public 192.168.1.1

# If successful, add to config and run
echo "test-router;192.168.1.1;public;161" >> configs/devices.conf
./bin/netmon
```

## Basic Usage

### Starting the Application

```bash
# Run from project directory
./bin/netmon

# Or if installed system-wide
netmon
```

### Main Menu Navigation

When the application starts, you'll see:

```
========================================
  Network Monitoring & Visualization  
  Tool for Cisco Networking Devices   
========================================

=== Main Menu ===
1. Start Device Monitoring
2. View Network Statistics
3. Configure Devices
4. View Network Topology
5. Show Alerts
6. Settings
0. Exit

Enter your choice:
```

**Menu Options**:
- **1**: Start monitoring configured devices
- **2**: Display current network statistics
- **3**: Manage device configuration
- **4**: View network topology diagram
- **5**: Review system alerts
- **6**: Adjust application settings
- **0**: Exit the application

### Example Session

```
Enter your choice: 1

=== Device Monitoring ===

Starting real-time monitoring...
Monitoring 3 devices...
- router1 (192.168.1.1): UP [Response: 2ms]
- switch1 (192.168.1.2): UP [Response: 1ms]
- firewall1 (192.168.1.254): UP [Response: 3ms]

Press Enter to return to main menu...
```

## Configuration Examples

### Example 1: Single Router

**Scenario**: Monitor one Cisco router

`configs/devices.conf`:
```
# Home network router
home-router;192.168.1.1;public;161
```

**Cisco Configuration**:
```cisco
! Enable SNMP on the router
config terminal
snmp-server community public RO
end
write memory
```

### Example 2: Small Office Network

**Scenario**: Monitor a router and two switches

`configs/devices.conf`:
```
# Small office network
office-router;192.168.1.1;mycompany;161
office-sw-floor1;192.168.1.10;mycompany;161
office-sw-floor2;192.168.1.11;mycompany;161
```

### Example 3: Enterprise Network

**Scenario**: Monitor multiple sites with different community strings

`configs/devices.conf`:
```
# Headquarters
hq-core-router;10.1.0.1;hq_readonly;161
hq-dist-sw-01;10.1.0.10;hq_readonly;161
hq-dist-sw-02;10.1.0.11;hq_readonly;161

# Branch Office 1
branch1-router;10.2.0.1;branch1_readonly;161
branch1-switch;10.2.0.10;branch1_readonly;161

# Branch Office 2
branch2-router;10.3.0.1;branch2_readonly;161
branch2-switch;10.3.0.10;branch2_readonly;161

# DMZ
dmz-firewall;192.168.100.1;dmz_readonly;161
```

### Example 4: Lab Environment

**Scenario**: Monitor simulated devices in GNS3

`configs/devices.conf`:
```
# GNS3 Lab Topology
lab-r1;172.16.1.1;cisco;161
lab-r2;172.16.1.2;cisco;161
lab-r3;172.16.1.3;cisco;161
lab-sw1;172.16.1.10;cisco;161
lab-sw2;172.16.1.11;cisco;161
```

## Common Scenarios

### Scenario 1: Monitoring Device Status

**Goal**: Check if devices are up and responsive

**Steps**:
1. Start the application
2. Select option 1 (Start Device Monitoring)
3. Observe device status updates
4. Look for response times and status indicators

**Expected Output**:
```
Device Status Report:
---------------------
✓ router1    (192.168.1.1)   - UP   - Response: 2ms
✓ switch1    (192.168.1.2)   - UP   - Response: 1ms
✗ firewall1  (192.168.1.254) - DOWN - Timeout
```

### Scenario 2: Viewing Network Statistics

**Goal**: Get an overview of network health

**Steps**:
1. Start monitoring (Option 1)
2. Let it run for a few minutes
3. Return to main menu
4. Select option 2 (View Network Statistics)

**Expected Output**:
```
Network Statistics:
------------------
Total Devices:        10
Active Devices:       9
Inactive Devices:     1
Average Response:     2.3ms
Total Data In:        1.2 GB
Total Data Out:       856 MB
```

### Scenario 3: Investigating Alerts

**Goal**: Review and troubleshoot network issues

**Steps**:
1. Select option 5 (Show Alerts)
2. Review alert messages
3. Note affected devices
4. Investigate high-priority alerts first

**Expected Output**:
```
Recent Alerts:
-------------
[CRITICAL] 2024-01-15 10:23:45 - firewall1: Device not responding
[WARNING]  2024-01-15 10:20:12 - switch1: High CPU usage (85%)
[INFO]     2024-01-15 10:15:33 - router1: Device restarted
```

### Scenario 4: Adding a New Device

**Goal**: Add a newly deployed device to monitoring

**Method 1: Edit Configuration File**
```bash
# Open config file
nano configs/devices.conf

# Add new device
new-switch;192.168.1.50;public;161

# Save and restart application
```

**Method 2: Using Application Menu** (Future feature)
```
Select option 3 (Configure Devices)
Choose "Add new device"
Enter hostname: new-switch
Enter IP: 192.168.1.50
Enter community: public
Enter port: 161
Save configuration
```

## Advanced Usage

### Custom Poll Intervals

**Future Feature**: Configure how often devices are polled

```c
// In future configuration
poll_interval_seconds = 30;  // Poll every 30 seconds
timeout_seconds = 5;         // Timeout after 5 seconds
max_retries = 3;             // Retry 3 times before marking down
```

### Filtering and Sorting

**Future Feature**: Filter devices by status or location

```
Show only:
[ ] UP devices
[ ] DOWN devices
[ ] WARNING status
[ ] All devices (default)

Sort by:
[ ] Hostname
[ ] IP Address
[ ] Response Time
[ ] Status
```

### Exporting Data

**Future Feature**: Export statistics and logs

```bash
# Export to CSV
./bin/netmon --export-stats stats.csv

# Export alerts
./bin/netmon --export-alerts alerts.log
```

### Automation with Scripts

**Example**: Automated monitoring script

```bash
#!/bin/bash
# monitor.sh - Automated monitoring script

# Start monitoring in background
./bin/netmon --daemon &
PID=$!

# Run for 1 hour
sleep 3600

# Stop monitoring
kill $PID

# Generate report
echo "Monitoring session completed"
```

## Troubleshooting Tips

### Tip 1: Device Not Responding

**Check**:
```bash
# Test network connectivity
ping 192.168.1.1

# Test SNMP access
snmpwalk -v2c -c public 192.168.1.1 system

# Verify firewall rules
sudo iptables -L -n | grep 161
```

### Tip 2: Incorrect Community String

**Symptoms**:
- Timeout errors
- Authentication failures
- No data returned

**Solution**:
```bash
# Test with different community strings
snmpwalk -v2c -c public 192.168.1.1
snmpwalk -v2c -c private 192.168.1.1

# Update devices.conf with correct string
```

### Tip 3: Permission Issues

**Symptoms**:
- Cannot bind to raw socket
- Permission denied errors

**Solution**:
```bash
# Grant capabilities (Linux)
sudo setcap cap_net_raw,cap_net_admin=eip ./bin/netmon

# Or run as root (not recommended)
sudo ./bin/netmon
```

### Tip 4: High CPU Usage

**Check**:
```bash
# Monitor resource usage
top -p $(pidof netmon)

# Review configuration
# - Reduce number of devices
# - Increase poll interval
# - Check for infinite loops
```

### Tip 5: Memory Leaks

**Debug**:
```bash
# Build debug version
make debug

# Run with valgrind
valgrind --leak-check=full ./bin/netmon

# Review output for leaks
```

## Best Practices

### 1. Security

- Use strong SNMP community strings
- Restrict SNMP access with ACLs
- Use read-only (RO) community strings
- Keep devices.conf file secure (chmod 600)

### 2. Performance

- Don't poll too frequently (30-60 seconds recommended)
- Limit number of devices per instance
- Monitor application resource usage
- Use appropriate timeout values

### 3. Reliability

- Regular backups of configuration
- Monitor the monitoring tool itself
- Have alerting for critical issues
- Document your network topology

### 4. Maintenance

- Regular updates to device list
- Review and clear old alerts
- Monitor log file sizes
- Test monitoring after network changes

## Additional Resources

- [Setup Guide](../docs/SETUP.md) - Installation instructions
- [Architecture](../docs/ARCHITECTURE.md) - System design
- [Main README](../README.md) - Project overview
- Cisco SNMP Configuration Guide
- Net-SNMP Documentation

## Getting Help

If you need assistance:

1. Check this usage guide
2. Review the setup documentation
3. Search GitHub issues
4. Open a new issue with:
   - Your configuration
   - Error messages
   - Steps to reproduce
   - System information

## Contributing Examples

Have a useful example? Please contribute!

1. Fork the repository
2. Add your example to this file
3. Submit a pull request
4. Help others learn from your experience!

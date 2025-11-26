/*
 * Network Monitoring and Visualization Tool
 * Main Header File
 * 
 * Common definitions, structures, and function declarations
 * for the network monitoring system.
 */

#ifndef NETMON_H
#define NETMON_H

#include <stdint.h>
#include <time.h>

/* Version information */
#define NETMON_VERSION_MAJOR 1
#define NETMON_VERSION_MINOR 0
#define NETMON_VERSION_PATCH 0

/* Configuration constants */
#define MAX_DEVICES 256
#define MAX_HOSTNAME_LEN 256
#define MAX_IP_LEN 16
#define MAX_COMMUNITY_LEN 64
#define MAX_ALERTS 1000
#define DEFAULT_SNMP_PORT 161
#define DEFAULT_TIMEOUT 5

/* Return codes */
#define NETMON_SUCCESS 0
#define NETMON_ERROR -1
#define NETMON_TIMEOUT -2
#define NETMON_NO_RESPONSE -3

/* Device status */
typedef enum {
    DEVICE_STATUS_UNKNOWN = 0,
    DEVICE_STATUS_UP,
    DEVICE_STATUS_DOWN,
    DEVICE_STATUS_WARNING
} device_status_t;

/* Alert severity levels */
typedef enum {
    ALERT_INFO = 0,
    ALERT_WARNING,
    ALERT_ERROR,
    ALERT_CRITICAL
} alert_severity_t;

/* Network device structure */
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

/* Alert structure */
typedef struct {
    time_t timestamp;
    alert_severity_t severity;
    char device_hostname[MAX_HOSTNAME_LEN];
    char message[256];
} alert_t;

/* Statistics structure */
typedef struct {
    uint32_t total_devices;
    uint32_t active_devices;
    uint32_t inactive_devices;
    uint64_t total_bytes_in;
    uint64_t total_bytes_out;
    uint32_t total_alerts;
    float avg_response_time;
} network_stats_t;

/* Core system functions */
int init_netmon(void);
void cleanup_netmon(void);
const char* get_version(void);

/* Device management functions */
int add_device(const network_device_t *device);
int remove_device(const char *hostname);
int get_device(const char *hostname, network_device_t *device);
int get_all_devices(network_device_t *devices, int max_count);
int update_device_status(const char *hostname, device_status_t status);

/* Monitoring functions */
int start_monitoring(void);
int stop_monitoring(void);
int poll_device(const char *hostname);
int poll_all_devices(void);

/* Statistics functions */
int get_statistics(network_stats_t *stats);
int reset_statistics(void);

/* Alert functions */
int add_alert(const alert_t *alert);
int get_alerts(alert_t *alerts, int max_count);
int clear_alerts(void);

/* Configuration functions */
int load_config(const char *filename);
int save_config(const char *filename);

/* Utility functions */
const char* status_to_string(device_status_t status);
const char* severity_to_string(alert_severity_t severity);
int is_valid_ip(const char *ip);
time_t get_current_time(void);

/* Network communication functions */
int snmp_get(const char *ip, const char *community, const char *oid, char *result, size_t result_size);
int snmp_walk(const char *ip, const char *community, const char *oid);
int ping_device(const char *ip, int *response_time);

/* Network discovery functions */
int discover_network(void);
int discover_network_quick(void);
int discover_multi_subnet(void);
int discover_custom_subnet(const char *subnet_cidr);
int discover_arp_cache(void);
int discover_snmp_arp(const char *router_ip, const char *community);
int discover_netstat(void);
int discover_efficient(void);
int discover_automatic(void);  /* Fully automatic - no input required */
int traceroute_discover(const char *target_ip, char gateways[][MAX_IP_LEN], int *gateway_count);
int scan_subnet(const char *network_addr, int prefix_len);
int get_local_network_info(char *local_ip, char *network_addr, char *netmask);
int get_discovered_count(void);
int get_discovered_host(int index, char *ip, int *response_time);

/* Visualization functions */
int init_display(void);
void cleanup_display(void);
void update_display(void);
void draw_topology(void);
void draw_statistics(const network_stats_t *stats);

#endif /* NETMON_H */

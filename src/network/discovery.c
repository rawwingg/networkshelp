/*
 * Network Discovery Module
 * Discovers reachable IP addresses on local and remote networks using ICMP ping
 * Supports multi-subnet discovery through traceroute and custom subnet scanning
 */

#define _POSIX_C_SOURCE 200809L

#include "netmon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <poll.h>

/* Maximum number of discovered hosts */
#define MAX_DISCOVERED_HOSTS 1024
#define MAX_SUBNETS 32
#define MAX_HOPS 30

/* Structure to hold discovered host information */
typedef struct {
    char ip_address[MAX_IP_LEN];
    int response_time_ms;
    int is_reachable;
} discovered_host_t;

/* Global array of discovered hosts */
static discovered_host_t discovered_hosts[MAX_DISCOVERED_HOSTS];
static int discovered_count = 0;

/*
 * Validate that a string contains only valid IP address characters
 * Returns 1 if valid, 0 if invalid
 */
static int validate_ip_string(const char *ip_addr)
{
    if (ip_addr == NULL || *ip_addr == '\0') {
        return 0;
    }
    
    for (const char *p = ip_addr; *p != '\0'; p++) {
        if ((*p < '0' || *p > '9') && *p != '.') {
            return 0;
        }
    }
    return 1;
}

/*
 * Ping using system command (portable, works without root)
 * Returns response time in ms, or -1 if unreachable
 */
static int ping_host_system(const char *ip_addr)
{
    char cmd[256];
    FILE *fp;
    char result[256];
    int response_time = -1;

    /* Validate IP address to prevent command injection */
    if (!validate_ip_string(ip_addr)) {
        return -1;
    }

    /* Use system ping with 1 packet, 1 second timeout */
    snprintf(cmd, sizeof(cmd), "ping -c 1 -W 1 %s 2>/dev/null", ip_addr);
    
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }

    /* Read output and look for time= */
    while (fgets(result, sizeof(result), fp) != NULL) {
        char *time_ptr = strstr(result, "time=");
        if (time_ptr != NULL) {
            float time_ms;
            if (sscanf(time_ptr, "time=%f", &time_ms) == 1) {
                response_time = (int)(time_ms + 0.5);
                if (response_time < 1) response_time = 1;
            }
            break;
        }
    }

    pclose(fp);
    return response_time;
}

/*
 * Get the local network interface information
 * Returns the network address and netmask
 */
int get_local_network_info(char *local_ip, char *network_addr, char *netmask)
{
    struct ifaddrs *ifaddr, *ifa;
    int found = 0;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    /* Walk through linked list of interfaces */
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        /* Only interested in IPv4 */
        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;

        /* Skip loopback interface */
        if (strcmp(ifa->ifa_name, "lo") == 0)
            continue;

        /* Get IP address */
        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        struct sockaddr_in *mask = (struct sockaddr_in *)ifa->ifa_netmask;

        inet_ntop(AF_INET, &addr->sin_addr, local_ip, MAX_IP_LEN);
        
        if (netmask != NULL && mask != NULL) {
            inet_ntop(AF_INET, &mask->sin_addr, netmask, MAX_IP_LEN);
        }

        /* Calculate network address */
        if (network_addr != NULL && mask != NULL) {
            struct in_addr net;
            net.s_addr = addr->sin_addr.s_addr & mask->sin_addr.s_addr;
            inet_ntop(AF_INET, &net, network_addr, MAX_IP_LEN);
        }

        found = 1;
        break;
    }

    freeifaddrs(ifaddr);
    return found ? 0 : -1;
}

/*
 * Parse IP address into octets
 */
static void parse_ip(const char *ip, int *octets)
{
    sscanf(ip, "%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]);
}

/*
 * Calculate the number of host bits from netmask
 */
static int get_host_bits(const char *netmask)
{
    int octets[4];
    parse_ip(netmask, octets);
    
    unsigned int mask = (unsigned int)((octets[0] << 24) | (octets[1] << 16) | 
                        (octets[2] << 8) | octets[3]);
    
    int host_bits = 0;
    unsigned int test = 1;
    while ((mask & test) == 0 && host_bits < 32) {
        host_bits++;
        test <<= 1;
    }
    
    return host_bits;
}

/*
 * Discover all reachable hosts on the local network
 * Returns the number of hosts discovered
 */
int discover_network(void)
{
    char local_ip[MAX_IP_LEN];
    char network_addr[MAX_IP_LEN];
    char netmask[MAX_IP_LEN];
    int net_octets[4], mask_octets[4];
    
    discovered_count = 0;

    printf("\n=== Network Discovery ===\n\n");

    /* Get local network information */
    if (get_local_network_info(local_ip, network_addr, netmask) != 0) {
        printf("Error: Could not get network interface information\n");
        return -1;
    }

    printf("Local IP Address: %s\n", local_ip);
    printf("Network Address:  %s\n", network_addr);
    printf("Subnet Mask:      %s\n", netmask);
    printf("\n");

    /* Parse addresses */
    parse_ip(network_addr, net_octets);
    parse_ip(netmask, mask_octets);

    /* Calculate scan range based on netmask */
    int host_bits = get_host_bits(netmask);
    int max_hosts = (1 << host_bits) - 2;  /* Exclude network and broadcast */
    
    /* Limit scan to reasonable size */
    if (max_hosts > 254) {
        max_hosts = 254;
        printf("Note: Large network detected, limiting scan to %d hosts\n", max_hosts);
    }

    printf("Scanning %d potential hosts...\n", max_hosts);
    printf("This may take a few minutes.\n\n");

    /* Progress display */
    printf("Progress: [");
    fflush(stdout);

    int progress_step = max_hosts / 20;
    if (progress_step < 1) progress_step = 1;

    /* Scan each IP in the range */
    for (int i = 1; i <= max_hosts && discovered_count < MAX_DISCOVERED_HOSTS; i++) {
        char target_ip[MAX_IP_LEN];
        
        /* Calculate target IP */
        int target_octets[4];
        target_octets[0] = net_octets[0];
        target_octets[1] = net_octets[1];
        target_octets[2] = net_octets[2];
        target_octets[3] = (net_octets[3] & mask_octets[3]) + i;
        
        /* Handle octet overflow for larger subnets */
        if (host_bits > 8) {
            target_octets[2] = net_octets[2] + ((i - 1) / 256);
            target_octets[3] = (net_octets[3] & mask_octets[3]) + ((i - 1) % 256) + 1;
        }
        
        snprintf(target_ip, sizeof(target_ip), "%d.%d.%d.%d",
                 target_octets[0], target_octets[1], 
                 target_octets[2], target_octets[3]);

        /* Try to ping the host */
        int response_time = ping_host_system(target_ip);
        
        if (response_time > 0) {
            strncpy(discovered_hosts[discovered_count].ip_address, 
                    target_ip, MAX_IP_LEN - 1);
            discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
            discovered_hosts[discovered_count].response_time_ms = response_time;
            discovered_hosts[discovered_count].is_reachable = 1;
            discovered_count++;
        }

        /* Update progress bar */
        if (i % progress_step == 0) {
            printf("=");
            fflush(stdout);
        }
    }

    printf("] Done!\n\n");

    /* Display results */
    printf("=== Discovery Results ===\n\n");
    printf("Found %d reachable host(s):\n\n", discovered_count);

    if (discovered_count > 0) {
        printf("%-18s %s\n", "IP Address", "Response Time");
        printf("%-18s %s\n", "----------", "-------------");
        
        for (int i = 0; i < discovered_count; i++) {
            printf("%-18s %d ms\n", 
                   discovered_hosts[i].ip_address,
                   discovered_hosts[i].response_time_ms);
        }
    } else {
        printf("No hosts found. This could be due to:\n");
        printf("- Firewall blocking ICMP packets\n");
        printf("- No other hosts on the network\n");
        printf("- Network configuration issues\n");
    }

    printf("\n");
    return discovered_count;
}

/*
 * Quick scan - only scan common host addresses (1-20)
 */
int discover_network_quick(void)
{
    char local_ip[MAX_IP_LEN];
    char network_addr[MAX_IP_LEN];
    char netmask[MAX_IP_LEN];
    int net_octets[4];
    
    discovered_count = 0;

    printf("\n=== Quick Network Discovery ===\n\n");

    if (get_local_network_info(local_ip, network_addr, netmask) != 0) {
        printf("Error: Could not get network interface information\n");
        return -1;
    }

    printf("Local IP: %s\n", local_ip);
    printf("Scanning local machine and common host addresses...\n\n");

    parse_ip(network_addr, net_octets);

    /* First check localhost */
    printf("Checking 127.0.0.1 (localhost)... ");
    fflush(stdout);
    int response_time = ping_host_system("127.0.0.1");
    if (response_time > 0) {
        printf("REACHABLE (%d ms)\n", response_time);
        strncpy(discovered_hosts[discovered_count].ip_address, 
                "127.0.0.1", MAX_IP_LEN - 1);
        discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
        discovered_hosts[discovered_count].response_time_ms = response_time;
        discovered_hosts[discovered_count].is_reachable = 1;
        discovered_count++;
    } else {
        printf("not reachable\n");
    }

    /* Check own IP */
    printf("Checking %s (self)... ", local_ip);
    fflush(stdout);
    response_time = ping_host_system(local_ip);
    if (response_time > 0) {
        printf("REACHABLE (%d ms)\n", response_time);
        strncpy(discovered_hosts[discovered_count].ip_address, 
                local_ip, MAX_IP_LEN - 1);
        discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
        discovered_hosts[discovered_count].response_time_ms = response_time;
        discovered_hosts[discovered_count].is_reachable = 1;
        discovered_count++;
    } else {
        printf("not reachable\n");
    }

    /* Quick scan targets */
    int targets[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 
                     11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                     100, 200, 254};
    int num_targets = (int)(sizeof(targets) / sizeof(targets[0]));

    for (int i = 0; i < num_targets && discovered_count < MAX_DISCOVERED_HOSTS; i++) {
        char target_ip[MAX_IP_LEN];
        
        snprintf(target_ip, sizeof(target_ip), "%d.%d.%d.%d",
                 net_octets[0], net_octets[1], net_octets[2], targets[i]);

        /* Skip if it's our own IP */
        if (strcmp(target_ip, local_ip) == 0) {
            continue;
        }

        printf("Checking %s... ", target_ip);
        fflush(stdout);

        response_time = ping_host_system(target_ip);
        
        if (response_time > 0) {
            printf("REACHABLE (%d ms)\n", response_time);
            strncpy(discovered_hosts[discovered_count].ip_address, 
                    target_ip, MAX_IP_LEN - 1);
            discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
            discovered_hosts[discovered_count].response_time_ms = response_time;
            discovered_hosts[discovered_count].is_reachable = 1;
            discovered_count++;
        } else {
            printf("not reachable\n");
        }
    }

    printf("\n=== Results ===\n");
    printf("Found %d reachable host(s)\n\n", discovered_count);

    return discovered_count;
}

/*
 * Get the count of discovered hosts
 */
int get_discovered_count(void)
{
    return discovered_count;
}

/*
 * Get discovered host by index
 */
int get_discovered_host(int index, char *ip, int *response_time)
{
    if (index < 0 || index >= discovered_count) {
        return -1;
    }
    
    if (ip != NULL) {
        strncpy(ip, discovered_hosts[index].ip_address, MAX_IP_LEN - 1);
        ip[MAX_IP_LEN - 1] = '\0';
    }
    
    if (response_time != NULL) {
        *response_time = discovered_hosts[index].response_time_ms;
    }
    
    return 0;
}

/*
 * Discover network path using traceroute
 * Returns routers/gateways in the path to a destination
 */
int traceroute_discover(const char *target_ip, char gateways[][MAX_IP_LEN], int *gateway_count)
{
    char cmd[256];
    FILE *fp;
    char result[512];
    *gateway_count = 0;

    /* Validate target IP */
    if (!validate_ip_string(target_ip)) {
        return -1;
    }

    printf("Tracing route to %s...\n", target_ip);

    /* Use traceroute with limited hops */
    snprintf(cmd, sizeof(cmd), "traceroute -n -m %d -w 1 %s 2>/dev/null", MAX_HOPS, target_ip);
    
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }

    /* Parse traceroute output */
    while (fgets(result, sizeof(result), fp) != NULL && *gateway_count < MAX_HOPS) {
        char ip[MAX_IP_LEN];
        int hop_num;
        
        /* Try to parse hop number and IP address */
        if (sscanf(result, " %d %15s", &hop_num, ip) >= 2) {
            /* Validate that we got an IP (not * for timeout) */
            if (validate_ip_string(ip)) {
                strncpy(gateways[*gateway_count], ip, MAX_IP_LEN - 1);
                gateways[*gateway_count][MAX_IP_LEN - 1] = '\0';
                printf("  Hop %d: %s\n", hop_num, ip);
                (*gateway_count)++;
            }
        }
    }

    pclose(fp);
    return *gateway_count;
}

/*
 * Scan a specific subnet
 */
int scan_subnet(const char *network_addr, int prefix_len)
{
    int net_octets[4];
    int hosts_found = 0;
    
    parse_ip(network_addr, net_octets);
    
    /* Calculate number of hosts based on prefix length */
    int host_bits = 32 - prefix_len;
    int max_hosts = (1 << host_bits) - 2;  /* Exclude network and broadcast */
    
    /* Limit scan to reasonable size */
    if (max_hosts > 254) {
        max_hosts = 254;
    }
    
    if (max_hosts <= 0) {
        return 0;
    }

    printf("\nScanning subnet %s/%d (%d hosts)...\n", network_addr, prefix_len, max_hosts);

    /* Scan each IP in the subnet */
    for (int i = 1; i <= max_hosts && discovered_count < MAX_DISCOVERED_HOSTS; i++) {
        char target_ip[MAX_IP_LEN];
        int target_octets[4];
        
        target_octets[0] = net_octets[0];
        target_octets[1] = net_octets[1];
        target_octets[2] = net_octets[2];
        target_octets[3] = i;
        
        /* Handle larger subnets */
        if (host_bits > 8) {
            target_octets[2] = net_octets[2] + ((i - 1) / 256);
            target_octets[3] = ((i - 1) % 256) + 1;
        }
        
        snprintf(target_ip, sizeof(target_ip), "%d.%d.%d.%d",
                 target_octets[0], target_octets[1], 
                 target_octets[2], target_octets[3]);

        int response_time = ping_host_system(target_ip);
        
        if (response_time > 0) {
            /* Check if already discovered */
            int already_found = 0;
            for (int j = 0; j < discovered_count; j++) {
                if (strcmp(discovered_hosts[j].ip_address, target_ip) == 0) {
                    already_found = 1;
                    break;
                }
            }
            
            if (!already_found) {
                strncpy(discovered_hosts[discovered_count].ip_address, 
                        target_ip, MAX_IP_LEN - 1);
                discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                discovered_hosts[discovered_count].response_time_ms = response_time;
                discovered_hosts[discovered_count].is_reachable = 1;
                discovered_count++;
                hosts_found++;
                printf("  Found: %s (%d ms)\n", target_ip, response_time);
            }
        }
        
        /* Show progress every 50 hosts */
        if (i % 50 == 0) {
            printf("  Progress: %d/%d hosts scanned...\n", i, max_hosts);
        }
    }
    
    return hosts_found;
}

/*
 * Discover networks across multiple subnets
 * Uses traceroute to find gateways, then scans discovered networks
 */
int discover_multi_subnet(void)
{
    char local_ip[MAX_IP_LEN];
    char network_addr[MAX_IP_LEN];
    char netmask[MAX_IP_LEN];
    char gateways[MAX_HOPS][MAX_IP_LEN];
    int gateway_count = 0;
    int total_found = 0;
    
    discovered_count = 0;

    printf("\n=== Multi-Subnet Network Discovery ===\n\n");

    /* Get local network information */
    if (get_local_network_info(local_ip, network_addr, netmask) != 0) {
        printf("Error: Could not get network interface information\n");
        return -1;
    }

    printf("Local IP Address: %s\n", local_ip);
    printf("Local Network:    %s\n", network_addr);
    printf("Subnet Mask:      %s\n\n", netmask);

    /* First, scan local subnet */
    printf("Step 1: Scanning local subnet...\n");
    int local_prefix = 24;  /* Default to /24 */
    
    /* Calculate actual prefix from netmask */
    int mask_octets[4];
    parse_ip(netmask, mask_octets);
    unsigned int mask_val = (unsigned int)((mask_octets[0] << 24) | (mask_octets[1] << 16) | 
                            (mask_octets[2] << 8) | mask_octets[3]);
    local_prefix = 0;
    while (mask_val & 0x80000000) {
        local_prefix++;
        mask_val <<= 1;
    }
    
    total_found += scan_subnet(network_addr, local_prefix);

    /* Try to discover gateway/router */
    printf("\nStep 2: Discovering network gateways...\n");
    
    /* Try common gateway addresses */
    char common_gateways[][MAX_IP_LEN] = {
        "8.8.8.8",      /* Google DNS - to trace internet path */
        "1.1.1.1"       /* Cloudflare DNS - alternative trace */
    };
    
    for (int i = 0; i < 2; i++) {
        if (traceroute_discover(common_gateways[i], gateways, &gateway_count) > 0) {
            printf("\nDiscovered %d gateways in path to %s\n", gateway_count, common_gateways[i]);
            
            /* For each gateway, try to determine its subnet and scan */
            for (int j = 0; j < gateway_count && j < 5; j++) {  /* Limit to first 5 hops */
                /* Extract network from gateway IP (assume /24) */
                int gw_octets[4];
                parse_ip(gateways[j], gw_octets);
                
                char gw_network[MAX_IP_LEN];
                snprintf(gw_network, sizeof(gw_network), "%d.%d.%d.0",
                         gw_octets[0], gw_octets[1], gw_octets[2]);
                
                /* Check if we already scanned this subnet */
                int already_scanned = 0;
                if (gw_octets[0] == 10 || 
                    (gw_octets[0] == 172 && gw_octets[1] >= 16 && gw_octets[1] <= 31) ||
                    (gw_octets[0] == 192 && gw_octets[1] == 168)) {
                    
                    /* Check against local network */
                    int local_octets[4];
                    parse_ip(network_addr, local_octets);
                    if (local_octets[0] == gw_octets[0] && 
                        local_octets[1] == gw_octets[1] &&
                        local_octets[2] == gw_octets[2]) {
                        already_scanned = 1;
                    }
                    
                    if (!already_scanned) {
                        printf("\nStep 3: Scanning remote subnet %s/24 (via gateway %s)...\n", 
                               gw_network, gateways[j]);
                        total_found += scan_subnet(gw_network, 24);
                    }
                }
            }
            break;  /* Only need one successful traceroute */
        }
    }

    /* Display final results */
    printf("\n=== Multi-Subnet Discovery Results ===\n\n");
    printf("Total hosts discovered: %d\n\n", discovered_count);

    if (discovered_count > 0) {
        printf("%-18s %s\n", "IP Address", "Response Time");
        printf("%-18s %s\n", "----------", "-------------");
        
        for (int i = 0; i < discovered_count; i++) {
            printf("%-18s %d ms\n", 
                   discovered_hosts[i].ip_address,
                   discovered_hosts[i].response_time_ms);
        }
    }

    printf("\n");
    return discovered_count;
}

/*
 * ARP-based discovery - Query local ARP cache for known hosts
 * This is much faster than ping scanning as it only shows hosts
 * that have recently communicated with this machine
 */
int discover_arp_cache(void)
{
    FILE *fp;
    char result[512];
    char ip[MAX_IP_LEN];
    char mac[32];
    char iface[32];
    
    discovered_count = 0;

    printf("\n=== ARP Cache Discovery ===\n\n");
    printf("Querying local ARP cache for known hosts...\n");
    printf("(This shows hosts that have recently communicated with this machine)\n\n");

    /* Try using 'ip neigh show' first (more modern) */
    fp = popen("ip neigh show 2>/dev/null || arp -a 2>/dev/null", "r");
    if (fp == NULL) {
        printf("Error: Could not query ARP cache\n");
        return -1;
    }

    printf("%-18s %-20s %s\n", "IP Address", "MAC Address", "State");
    printf("%-18s %-20s %s\n", "----------", "-----------", "-----");

    while (fgets(result, sizeof(result), fp) != NULL && discovered_count < MAX_DISCOVERED_HOSTS) {
        /* Try to parse 'ip neigh' format: IP dev IFACE lladdr MAC STATE */
        char state[32] = "";
        if (sscanf(result, "%15s dev %31s lladdr %31s %31s", ip, iface, mac, state) >= 3) {
            if (validate_ip_string(ip) && strcmp(state, "FAILED") != 0) {
                printf("%-18s %-20s %s\n", ip, mac, state[0] ? state : "REACHABLE");
                strncpy(discovered_hosts[discovered_count].ip_address, ip, MAX_IP_LEN - 1);
                discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                discovered_hosts[discovered_count].response_time_ms = 0;
                discovered_hosts[discovered_count].is_reachable = 1;
                discovered_count++;
            }
        }
        /* Try 'arp -a' format: hostname (IP) at MAC [ether] on iface */
        else if (sscanf(result, "%*s (%15[^)]) at %31s", ip, mac) >= 2) {
            if (validate_ip_string(ip) && strcmp(mac, "<incomplete>") != 0) {
                printf("%-18s %-20s %s\n", ip, mac, "REACHABLE");
                strncpy(discovered_hosts[discovered_count].ip_address, ip, MAX_IP_LEN - 1);
                discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                discovered_hosts[discovered_count].response_time_ms = 0;
                discovered_hosts[discovered_count].is_reachable = 1;
                discovered_count++;
            }
        }
    }

    pclose(fp);

    printf("\n=== Results ===\n");
    printf("Found %d host(s) in ARP cache\n\n", discovered_count);

    return discovered_count;
}

/*
 * SNMP-based discovery - Query router ARP table via SNMP
 * Requires SNMP community string (typically "public" for read access)
 * OID: 1.3.6.1.2.1.4.22.1.2 - ipNetToMediaPhysAddress (ARP table)
 */
int discover_snmp_arp(const char *router_ip, const char *community)
{
    char cmd[512];
    FILE *fp;
    char result[512];
    int hosts_found = 0;

    /* Validate inputs */
    if (!validate_ip_string(router_ip)) {
        printf("Error: Invalid router IP address\n");
        return -1;
    }

    /* Validate community string - only allow alphanumeric and basic chars */
    for (const char *p = community; *p != '\0'; p++) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || 
              (*p >= '0' && *p <= '9') || *p == '_' || *p == '-')) {
            printf("Error: Invalid community string\n");
            return -1;
        }
    }

    printf("\n=== SNMP ARP Table Discovery ===\n\n");
    printf("Querying ARP table on router %s...\n", router_ip);
    printf("Community: %s\n\n", community);

    /* Use snmpwalk to query ARP table */
    /* OID: 1.3.6.1.2.1.4.22.1.3 - ipNetToMediaNetAddress (IP addresses in ARP) */
    snprintf(cmd, sizeof(cmd), 
             "snmpwalk -v2c -c %s %s 1.3.6.1.2.1.4.22.1.3 2>/dev/null",
             community, router_ip);
    
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Error: Could not execute SNMP query\n");
        return -1;
    }

    printf("%-18s %s\n", "IP Address", "Source");
    printf("%-18s %s\n", "----------", "------");

    while (fgets(result, sizeof(result), fp) != NULL && discovered_count < MAX_DISCOVERED_HOSTS) {
        /* Parse SNMP output - look for IP addresses */
        char *ip_start = strstr(result, "IpAddress:");
        if (ip_start != NULL) {
            ip_start += 10;  /* Skip "IpAddress:" */
            while (*ip_start == ' ') ip_start++;
            
            char ip[MAX_IP_LEN];
            if (sscanf(ip_start, "%15s", ip) == 1 && validate_ip_string(ip)) {
                /* Check if already discovered */
                int already_found = 0;
                for (int j = 0; j < discovered_count; j++) {
                    if (strcmp(discovered_hosts[j].ip_address, ip) == 0) {
                        already_found = 1;
                        break;
                    }
                }
                
                if (!already_found) {
                    printf("%-18s Router ARP\n", ip);
                    strncpy(discovered_hosts[discovered_count].ip_address, ip, MAX_IP_LEN - 1);
                    discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                    discovered_hosts[discovered_count].response_time_ms = 0;
                    discovered_hosts[discovered_count].is_reachable = 1;
                    discovered_count++;
                    hosts_found++;
                }
            }
        }
    }

    pclose(fp);

    if (hosts_found == 0) {
        printf("No hosts found. This could mean:\n");
        printf("- SNMP is not enabled on the router\n");
        printf("- Incorrect community string\n");
        printf("- snmpwalk tool is not installed (install with: apt install snmp)\n");
    }

    printf("\n=== Results ===\n");
    printf("Found %d host(s) via SNMP\n\n", hosts_found);

    return hosts_found;
}

/*
 * Passive discovery using netstat - find hosts we're communicating with
 */
int discover_netstat(void)
{
    FILE *fp;
    char result[512];
    
    discovered_count = 0;

    printf("\n=== Active Connections Discovery ===\n\n");
    printf("Finding hosts with active connections...\n\n");

    /* Use netstat or ss to find established connections */
    fp = popen("ss -tn state established 2>/dev/null || netstat -tn 2>/dev/null | grep ESTABLISHED", "r");
    if (fp == NULL) {
        printf("Error: Could not query network connections\n");
        return -1;
    }

    printf("%-18s %-8s %s\n", "Remote IP", "Port", "State");
    printf("%-18s %-8s %s\n", "---------", "----", "-----");

    while (fgets(result, sizeof(result), fp) != NULL && discovered_count < MAX_DISCOVERED_HOSTS) {
        char local_addr[64], remote_addr[64];
        
        /* Parse ss output format */
        if (sscanf(result, "%*s %*s %*s %63s %63s", local_addr, remote_addr) >= 2) {
            /* Extract IP from IP:port format */
            char *colon = strrchr(remote_addr, ':');
            if (colon != NULL) {
                *colon = '\0';
                char *ip = remote_addr;
                char *port = colon + 1;
                
                /* Handle IPv6-mapped IPv4 (::ffff:IP) */
                if (strncmp(ip, "::ffff:", 7) == 0) {
                    ip += 7;
                }
                
                /* Skip localhost and IPv6 */
                if (validate_ip_string(ip) && strcmp(ip, "127.0.0.1") != 0) {
                    /* Check if already discovered */
                    int already_found = 0;
                    for (int j = 0; j < discovered_count; j++) {
                        if (strcmp(discovered_hosts[j].ip_address, ip) == 0) {
                            already_found = 1;
                            break;
                        }
                    }
                    
                    if (!already_found) {
                        printf("%-18s %-8s ESTABLISHED\n", ip, port);
                        strncpy(discovered_hosts[discovered_count].ip_address, ip, MAX_IP_LEN - 1);
                        discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                        discovered_hosts[discovered_count].response_time_ms = 0;
                        discovered_hosts[discovered_count].is_reachable = 1;
                        discovered_count++;
                    }
                }
            }
        }
    }

    pclose(fp);

    printf("\n=== Results ===\n");
    printf("Found %d unique remote host(s) with active connections\n\n", discovered_count);

    return discovered_count;
}

/*
 * Combined efficient discovery - uses all non-bruteforce methods
 */
int discover_efficient(void)
{
    discovered_count = 0;

    printf("\n=== Efficient Network Discovery ===\n\n");
    printf("This combines multiple discovery methods WITHOUT brute-force scanning:\n");
    printf("1. ARP Cache - Hosts that recently communicated with us\n");
    printf("2. Active Connections - Hosts we currently have connections to\n");
    printf("3. Default Gateway - Network router/gateway\n\n");

    /* Step 1: ARP Cache */
    printf("--- Step 1: Checking ARP Cache ---\n");
    discover_arp_cache();

    /* Step 2: Active Connections (netstat) */
    printf("\n--- Step 2: Checking Active Connections ---\n");
    int before = discovered_count;
    
    FILE *fp = popen("ss -tn state established 2>/dev/null || netstat -tn 2>/dev/null | grep ESTABLISHED", "r");
    if (fp != NULL) {
        char result[512];
        while (fgets(result, sizeof(result), fp) != NULL && discovered_count < MAX_DISCOVERED_HOSTS) {
            char local_addr[64], remote_addr[64];
            if (sscanf(result, "%*s %*s %*s %63s %63s", local_addr, remote_addr) >= 2) {
                char *colon = strrchr(remote_addr, ':');
                if (colon != NULL) {
                    *colon = '\0';
                    char *ip = remote_addr;
                    if (strncmp(ip, "::ffff:", 7) == 0) ip += 7;
                    
                    if (validate_ip_string(ip) && strcmp(ip, "127.0.0.1") != 0) {
                        int already_found = 0;
                        for (int j = 0; j < discovered_count; j++) {
                            if (strcmp(discovered_hosts[j].ip_address, ip) == 0) {
                                already_found = 1;
                                break;
                            }
                        }
                        if (!already_found) {
                            strncpy(discovered_hosts[discovered_count].ip_address, ip, MAX_IP_LEN - 1);
                            discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                            discovered_hosts[discovered_count].response_time_ms = 0;
                            discovered_hosts[discovered_count].is_reachable = 1;
                            discovered_count++;
                        }
                    }
                }
            }
        }
        pclose(fp);
    }
    printf("Found %d new host(s) from active connections\n", discovered_count - before);

    /* Step 3: Default Gateway */
    printf("\n--- Step 3: Finding Default Gateway ---\n");
    fp = popen("ip route | grep default | awk '{print $3}' 2>/dev/null || route -n | grep '^0.0.0.0' | awk '{print $2}'", "r");
    if (fp != NULL) {
        char gateway[MAX_IP_LEN];
        if (fgets(gateway, sizeof(gateway), fp) != NULL) {
            /* Remove newline */
            size_t len = strlen(gateway);
            if (len > 0 && gateway[len-1] == '\n') gateway[len-1] = '\0';
            
            if (validate_ip_string(gateway)) {
                int already_found = 0;
                for (int j = 0; j < discovered_count; j++) {
                    if (strcmp(discovered_hosts[j].ip_address, gateway) == 0) {
                        already_found = 1;
                        break;
                    }
                }
                if (!already_found) {
                    printf("Default Gateway: %s\n", gateway);
                    strncpy(discovered_hosts[discovered_count].ip_address, gateway, MAX_IP_LEN - 1);
                    discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                    discovered_hosts[discovered_count].response_time_ms = 0;
                    discovered_hosts[discovered_count].is_reachable = 1;
                    discovered_count++;
                }
            }
        }
        pclose(fp);
    }

    /* Display all discovered hosts */
    printf("\n=== Combined Discovery Results ===\n\n");
    printf("Total unique hosts discovered: %d\n\n", discovered_count);

    if (discovered_count > 0) {
        printf("%-18s %s\n", "IP Address", "Discovery Method");
        printf("%-18s %s\n", "----------", "----------------");
        
        for (int i = 0; i < discovered_count; i++) {
            printf("%-18s %s\n", 
                   discovered_hosts[i].ip_address,
                   "ARP/Connection/Gateway");
        }
    }

    printf("\nNote: This method only finds hosts that:\n");
    printf("- Have recently communicated with this machine (ARP)\n");
    printf("- Currently have active connections\n");
    printf("- Are in the routing path (gateway)\n");
    printf("\nFor complete subnet discovery, use brute-force scan options.\n\n");

    return discovered_count;
}

/*
 * Get default gateway IP address
 * Returns 1 on success, 0 on failure
 */
static int get_default_gateway(char *gateway, size_t gateway_size)
{
    FILE *fp;
    
    fp = popen("ip route | grep default | awk '{print $3}' 2>/dev/null || route -n | grep '^0.0.0.0' | awk '{print $2}'", "r");
    if (fp == NULL) {
        return 0;
    }
    
    if (fgets(gateway, (int)gateway_size, fp) != NULL) {
        /* Remove newline */
        size_t len = strlen(gateway);
        if (len > 0 && gateway[len-1] == '\n') gateway[len-1] = '\0';
        pclose(fp);
        return validate_ip_string(gateway);
    }
    
    pclose(fp);
    return 0;
}

/*
 * Check if SNMP community string is valid
 */
static int validate_community_string(const char *community)
{
    if (community == NULL || *community == '\0') return 0;
    
    for (const char *p = community; *p != '\0'; p++) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || 
              (*p >= '0' && *p <= '9') || *p == '_' || *p == '-')) {
            return 0;
        }
    }
    return 1;
}

/*
 * Try SNMP query with a specific community string
 * Returns number of hosts found, 0 if SNMP fails or no hosts
 */
static int try_snmp_community(const char *router_ip, const char *community)
{
    char cmd[512];
    FILE *fp;
    char result[512];
    int hosts_found = 0;

    /* Validate inputs */
    if (!validate_ip_string(router_ip)) {
        return 0;
    }

    if (!validate_community_string(community)) {
        return 0;
    }

    /* Try SNMP query - query ARP table */
    snprintf(cmd, sizeof(cmd), 
             "timeout 3 snmpwalk -v2c -c %s %s 1.3.6.1.2.1.4.22.1.3 2>/dev/null",
             community, router_ip);
    
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return 0;
    }

    while (fgets(result, sizeof(result), fp) != NULL && discovered_count < MAX_DISCOVERED_HOSTS) {
        /* Parse SNMP output - look for IP addresses */
        char *ip_start = strstr(result, "IpAddress:");
        if (ip_start != NULL) {
            ip_start += 10;
            while (*ip_start == ' ') ip_start++;
            
            char ip[MAX_IP_LEN];
            if (sscanf(ip_start, "%15s", ip) == 1 && validate_ip_string(ip)) {
                /* Check if already discovered */
                int already_found = 0;
                for (int j = 0; j < discovered_count; j++) {
                    if (strcmp(discovered_hosts[j].ip_address, ip) == 0) {
                        already_found = 1;
                        break;
                    }
                }
                
                if (!already_found) {
                    strncpy(discovered_hosts[discovered_count].ip_address, ip, MAX_IP_LEN - 1);
                    discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                    discovered_hosts[discovered_count].response_time_ms = 0;
                    discovered_hosts[discovered_count].is_reachable = 1;
                    discovered_count++;
                    hosts_found++;
                }
            }
        }
    }

    pclose(fp);
    return hosts_found;
}

/*
 * Discover next-hop routers from a router's routing table via SNMP
 * OID: 1.3.6.1.2.1.4.21.1.7 - ipRouteNextHop
 * Returns number of next-hop routers discovered
 */
static int discover_nexthop_routers(const char *router_ip, const char *community, 
                                    char nexthops[][MAX_IP_LEN], int *nexthop_count, int max_nexthops)
{
    char cmd[512];
    FILE *fp;
    char result[512];
    
    *nexthop_count = 0;

    if (!validate_ip_string(router_ip) || !validate_community_string(community)) {
        return 0;
    }

    /* Query routing table for next-hop addresses */
    snprintf(cmd, sizeof(cmd), 
             "timeout 3 snmpwalk -v2c -c %s %s 1.3.6.1.2.1.4.21.1.7 2>/dev/null",
             community, router_ip);
    
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return 0;
    }

    while (fgets(result, sizeof(result), fp) != NULL && *nexthop_count < max_nexthops) {
        /* Parse SNMP output for next-hop IP addresses */
        char *ip_start = strstr(result, "IpAddress:");
        if (ip_start != NULL) {
            ip_start += 10;
            while (*ip_start == ' ') ip_start++;
            
            char ip[MAX_IP_LEN];
            if (sscanf(ip_start, "%15s", ip) == 1 && validate_ip_string(ip)) {
                /* Skip 0.0.0.0 (directly connected routes) and our own IP */
                if (strcmp(ip, "0.0.0.0") == 0) continue;
                
                /* Check if it's a valid private IP (another router) */
                int octets[4];
                parse_ip(ip, octets);
                int is_private = (octets[0] == 10) || 
                                 (octets[0] == 172 && octets[1] >= 16 && octets[1] <= 31) ||
                                 (octets[0] == 192 && octets[1] == 168);
                
                if (!is_private) continue;
                
                /* Check if already in nexthops list or same as queried router */
                if (strcmp(ip, router_ip) == 0) continue;
                
                int already_found = 0;
                for (int j = 0; j < *nexthop_count; j++) {
                    if (strcmp(nexthops[j], ip) == 0) {
                        already_found = 1;
                        break;
                    }
                }
                
                if (!already_found) {
                    strncpy(nexthops[*nexthop_count], ip, MAX_IP_LEN - 1);
                    nexthops[*nexthop_count][MAX_IP_LEN - 1] = '\0';
                    (*nexthop_count)++;
                }
            }
        }
    }

    pclose(fp);
    return *nexthop_count;
}

/*
 * Discover router interfaces via SNMP
 * OID: 1.3.6.1.2.1.4.20.1.1 - ipAdEntAddr (IP addresses configured on router interfaces)
 * Returns number of interfaces discovered
 */
static int discover_router_interfaces(const char *router_ip, const char *community,
                                     char interfaces[][MAX_IP_LEN], int *iface_count, int max_ifaces)
{
    char cmd[512];
    FILE *fp;
    char result[512];
    
    *iface_count = 0;

    if (!validate_ip_string(router_ip) || !validate_community_string(community)) {
        return 0;
    }

    /* Query router interface IP addresses */
    snprintf(cmd, sizeof(cmd), 
             "timeout 3 snmpwalk -v2c -c %s %s 1.3.6.1.2.1.4.20.1.1 2>/dev/null",
             community, router_ip);
    
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return 0;
    }

    while (fgets(result, sizeof(result), fp) != NULL && *iface_count < max_ifaces) {
        /* Parse SNMP output for interface IP addresses */
        char *ip_start = strstr(result, "IpAddress:");
        if (ip_start != NULL) {
            ip_start += 10;
            while (*ip_start == ' ') ip_start++;
            
            char ip[MAX_IP_LEN];
            if (sscanf(ip_start, "%15s", ip) == 1 && validate_ip_string(ip)) {
                /* Skip loopback addresses */
                if (strncmp(ip, "127.", 4) == 0) continue;
                
                /* Check if already in list */
                int already_found = 0;
                for (int j = 0; j < *iface_count; j++) {
                    if (strcmp(interfaces[j], ip) == 0) {
                        already_found = 1;
                        break;
                    }
                }
                
                if (!already_found) {
                    strncpy(interfaces[*iface_count], ip, MAX_IP_LEN - 1);
                    interfaces[*iface_count][MAX_IP_LEN - 1] = '\0';
                    (*iface_count)++;
                }
            }
        }
    }

    pclose(fp);
    return *iface_count;
}

/*
 * Fully automatic network discovery
 * Discovers everything without requiring any user input:
 * 1. Finds default gateway automatically
 * 2. Tries SNMP with common community strings on gateway
 * 3. Uses SNMP to discover router interfaces and next-hop routers (better than traceroute!)
 * 4. Queries SNMP on discovered routers to find hosts in other subnets
 * 5. Falls back to ARP + connections if SNMP fails
 */
int discover_automatic(void)
{
    char gateway[MAX_IP_LEN];
    char routers_to_query[MAX_HOPS][MAX_IP_LEN];
    int routers_queried = 0;
    int routers_to_check = 0;
    char working_community[64] = "";
    int snmp_success = 0;
    int total_hosts = 0;
    
    /* Common SNMP community strings to try */
    const char *communities[] = {"abc", "public", "private", "community", "cisco", NULL};
    
    discovered_count = 0;

    printf("\n");
    printf("============================================\n");
    printf("     AUTOMATIC NETWORK DISCOVERY           \n");
    printf("============================================\n\n");
    printf("Discovering all reachable hosts automatically...\n");
    printf("Including hosts in OTHER SUBNETS across routers!\n");
    printf("No configuration required!\n\n");

    /* Step 1: Get default gateway */
    printf("--- Step 1: Finding Default Gateway ---\n");
    if (!get_default_gateway(gateway, sizeof(gateway))) {
        printf("Warning: Could not detect default gateway\n");
        printf("Falling back to ARP-only discovery...\n\n");
        goto arp_fallback;
    }
    printf("Default Gateway: %s\n\n", gateway);

    /* Add gateway to routers to query */
    strncpy(routers_to_query[routers_to_check], gateway, MAX_IP_LEN - 1);
    routers_to_query[routers_to_check][MAX_IP_LEN - 1] = '\0';
    routers_to_check++;

    /* Add gateway to discovered hosts */
    strncpy(discovered_hosts[discovered_count].ip_address, gateway, MAX_IP_LEN - 1);
    discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
    discovered_hosts[discovered_count].response_time_ms = 0;
    discovered_hosts[discovered_count].is_reachable = 1;
    discovered_count++;

    /* Step 2: Query routers using SNMP (iteratively discover more routers) */
    printf("--- Step 2: Discovering Routers and Hosts via SNMP ---\n");
    printf("Using SNMP to query router interfaces, routing tables, and ARP tables...\n");
    printf("This method discovers ALL connected networks, not just the ones facing us!\n\n");
    
    while (routers_queried < routers_to_check && routers_queried < MAX_HOPS) {
        char *current_router = routers_to_query[routers_queried];
        int found_community = 0;
        
        printf("Querying Router %d: %s\n", routers_queried + 1, current_router);
        
        /* Try each community string */
        for (int i = 0; communities[i] != NULL && !found_community; i++) {
            printf("  Trying community '%s'... ", communities[i]);
            fflush(stdout);
            
            /* First try to get router interfaces */
            char interfaces[MAX_HOPS][MAX_IP_LEN];
            int iface_count = 0;
            discover_router_interfaces(current_router, communities[i], interfaces, &iface_count, MAX_HOPS);
            
            if (iface_count > 0) {
                printf("SUCCESS!\n");
                found_community = 1;
                snmp_success = 1;
                strncpy(working_community, communities[i], sizeof(working_community) - 1);
                working_community[sizeof(working_community) - 1] = '\0';
                
                /* Add all router interfaces to discovered hosts */
                printf("  Router interfaces found: %d\n", iface_count);
                for (int j = 0; j < iface_count && discovered_count < MAX_DISCOVERED_HOSTS; j++) {
                    printf("    Interface: %s\n", interfaces[j]);
                    
                    /* Check if already discovered */
                    int already_found = 0;
                    for (int k = 0; k < discovered_count; k++) {
                        if (strcmp(discovered_hosts[k].ip_address, interfaces[j]) == 0) {
                            already_found = 1;
                            break;
                        }
                    }
                    if (!already_found) {
                        strncpy(discovered_hosts[discovered_count].ip_address, interfaces[j], MAX_IP_LEN - 1);
                        discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                        discovered_hosts[discovered_count].response_time_ms = 0;
                        discovered_hosts[discovered_count].is_reachable = 1;
                        discovered_count++;
                    }
                }
                
                /* Get next-hop routers from routing table */
                char nexthops[MAX_HOPS][MAX_IP_LEN];
                int nexthop_count = 0;
                discover_nexthop_routers(current_router, communities[i], nexthops, &nexthop_count, MAX_HOPS);
                
                if (nexthop_count > 0) {
                    printf("  Next-hop routers found: %d\n", nexthop_count);
                    for (int j = 0; j < nexthop_count; j++) {
                        printf("    Next-hop: %s\n", nexthops[j]);
                        
                        /* Add to routers to query if not already in list */
                        int already_in_list = 0;
                        for (int k = 0; k < routers_to_check; k++) {
                            if (strcmp(routers_to_query[k], nexthops[j]) == 0) {
                                already_in_list = 1;
                                break;
                            }
                        }
                        if (!already_in_list && routers_to_check < MAX_HOPS) {
                            strncpy(routers_to_query[routers_to_check], nexthops[j], MAX_IP_LEN - 1);
                            routers_to_query[routers_to_check][MAX_IP_LEN - 1] = '\0';
                            routers_to_check++;
                        }
                        
                        /* Also add next-hop to discovered hosts */
                        int already_found = 0;
                        for (int k = 0; k < discovered_count; k++) {
                            if (strcmp(discovered_hosts[k].ip_address, nexthops[j]) == 0) {
                                already_found = 1;
                                break;
                            }
                        }
                        if (!already_found && discovered_count < MAX_DISCOVERED_HOSTS) {
                            strncpy(discovered_hosts[discovered_count].ip_address, nexthops[j], MAX_IP_LEN - 1);
                            discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                            discovered_hosts[discovered_count].response_time_ms = 0;
                            discovered_hosts[discovered_count].is_reachable = 1;
                            discovered_count++;
                        }
                    }
                }
                
                /* Get hosts from ARP table */
                int arp_hosts = try_snmp_community(current_router, communities[i]);
                if (arp_hosts > 0) {
                    printf("  Hosts in ARP table: %d\n", arp_hosts);
                }
            } else {
                printf("no response\n");
            }
        }
        
        if (!found_community) {
            printf("  No SNMP access (router may use different credentials)\n");
        }
        
        printf("\n");
        routers_queried++;
    }

    if (!snmp_success) {
        printf("SNMP not available on any router.\n");
        printf("Routers may not support SNMP or use different credentials.\n");
        printf("Continuing with local discovery methods...\n");
    } else {
        printf("Total routers queried: %d\n\n", routers_queried);
    }

arp_fallback:
    /* Step 3: Local ARP cache */
    printf("--- Step 3: Checking Local ARP Cache ---\n");
    
    FILE *fp = popen("ip neigh show 2>/dev/null || arp -a 2>/dev/null", "r");
    if (fp != NULL) {
        char result[512];
        char ip[MAX_IP_LEN];
        char iface[32];
        char mac[32];
        char state[32] = "";
        int arp_count = 0;
        
        while (fgets(result, sizeof(result), fp) != NULL && discovered_count < MAX_DISCOVERED_HOSTS) {
            if (sscanf(result, "%15s dev %31s lladdr %31s %31s", ip, iface, mac, state) >= 3) {
                if (validate_ip_string(ip) && strcmp(state, "FAILED") != 0) {
                    /* Check if already discovered */
                    int already_found = 0;
                    for (int j = 0; j < discovered_count; j++) {
                        if (strcmp(discovered_hosts[j].ip_address, ip) == 0) {
                            already_found = 1;
                            break;
                        }
                    }
                    if (!already_found) {
                        strncpy(discovered_hosts[discovered_count].ip_address, ip, MAX_IP_LEN - 1);
                        discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                        discovered_hosts[discovered_count].response_time_ms = 0;
                        discovered_hosts[discovered_count].is_reachable = 1;
                        discovered_count++;
                        arp_count++;
                    }
                }
            }
            else if (sscanf(result, "%*s (%15[^)]) at %31s", ip, mac) >= 2) {
                if (validate_ip_string(ip) && strcmp(mac, "<incomplete>") != 0) {
                    int already_found = 0;
                    for (int j = 0; j < discovered_count; j++) {
                        if (strcmp(discovered_hosts[j].ip_address, ip) == 0) {
                            already_found = 1;
                            break;
                        }
                    }
                    if (!already_found) {
                        strncpy(discovered_hosts[discovered_count].ip_address, ip, MAX_IP_LEN - 1);
                        discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                        discovered_hosts[discovered_count].response_time_ms = 0;
                        discovered_hosts[discovered_count].is_reachable = 1;
                        discovered_count++;
                        arp_count++;
                    }
                }
            }
        }
        pclose(fp);
        printf("Found %d hosts in local ARP cache\n", arp_count);
    }

    /* Step 4: Active connections */
    printf("\n--- Step 4: Checking Active Network Connections ---\n");
    
    fp = popen("ss -tn state established 2>/dev/null || netstat -tn 2>/dev/null | grep ESTABLISHED", "r");
    if (fp != NULL) {
        char result[512];
        int conn_count = 0;
        
        while (fgets(result, sizeof(result), fp) != NULL && discovered_count < MAX_DISCOVERED_HOSTS) {
            char local_addr[64], remote_addr[64];
            if (sscanf(result, "%*s %*s %*s %63s %63s", local_addr, remote_addr) >= 2) {
                char *colon = strrchr(remote_addr, ':');
                if (colon != NULL) {
                    *colon = '\0';
                    char *ip = remote_addr;
                    if (strncmp(ip, "::ffff:", 7) == 0) ip += 7;
                    
                    if (validate_ip_string(ip) && strcmp(ip, "127.0.0.1") != 0) {
                        int already_found = 0;
                        for (int j = 0; j < discovered_count; j++) {
                            if (strcmp(discovered_hosts[j].ip_address, ip) == 0) {
                                already_found = 1;
                                break;
                            }
                        }
                        if (!already_found) {
                            strncpy(discovered_hosts[discovered_count].ip_address, ip, MAX_IP_LEN - 1);
                            discovered_hosts[discovered_count].ip_address[MAX_IP_LEN - 1] = '\0';
                            discovered_hosts[discovered_count].response_time_ms = 0;
                            discovered_hosts[discovered_count].is_reachable = 1;
                            discovered_count++;
                            conn_count++;
                        }
                    }
                }
            }
        }
        pclose(fp);
        printf("Found %d hosts with active connections\n", conn_count);
    }

    total_hosts = discovered_count;

    /* Final Results */
    printf("\n");
    printf("============================================\n");
    printf("          DISCOVERY RESULTS                \n");
    printf("============================================\n\n");
    
    printf("Total unique hosts discovered: %d\n\n", total_hosts);

    if (total_hosts > 0) {
        printf("%-18s %s\n", "IP Address", "Source");
        printf("%-18s %s\n", "----------", "------");
        
        for (int i = 0; i < discovered_count; i++) {
            printf("%-18s %s\n", 
                   discovered_hosts[i].ip_address,
                   (i == 0 && gateway[0]) ? "Gateway" : 
                   (snmp_success ? "Router ARP/Local" : "Local ARP/Conn"));
        }
    }

    printf("\n");
    if (snmp_success) {
        printf("SUCCESS: SNMP discovery worked - showing hosts from router(s) ARP table(s)\n");
        printf("         This includes hosts from ALL subnets the router(s) know about.\n");
    } else {
        printf("NOTE: SNMP was not available on any discovered router.\n");
        printf("      Only showing hosts this machine has directly communicated with.\n");
        printf("      To discover more hosts, ensure SNMP is enabled on your routers\n");
        printf("      with community string 'abc' or configure appropriately.\n");
    }
    printf("\n");

    return total_hosts;
}

/*
 * Scan a custom subnet specified by user
 */
int discover_custom_subnet(const char *subnet_cidr)
{
    char network[MAX_IP_LEN];
    int prefix_len = 24;  /* Default */
    
    discovered_count = 0;

    printf("\n=== Custom Subnet Discovery ===\n\n");

    /* Parse CIDR notation (e.g., "192.168.2.0/24") */
    const char *slash = strchr(subnet_cidr, '/');
    if (slash != NULL) {
        size_t ip_len = (size_t)(slash - subnet_cidr);
        if (ip_len >= MAX_IP_LEN) ip_len = MAX_IP_LEN - 1;
        strncpy(network, subnet_cidr, ip_len);
        network[ip_len] = '\0';
        
        /* Parse prefix with error checking */
        char *endptr;
        long parsed_prefix = strtol(slash + 1, &endptr, 10);
        if (*endptr != '\0' && *endptr != '\n' && *endptr != ' ') {
            printf("Error: Invalid prefix length format\n");
            return -1;
        }
        prefix_len = (int)parsed_prefix;
        
        if (prefix_len < 16 || prefix_len > 30) {
            printf("Error: Prefix length must be between 16 and 30\n");
            return -1;
        }
    } else {
        strncpy(network, subnet_cidr, MAX_IP_LEN - 1);
        network[MAX_IP_LEN - 1] = '\0';
    }

    /* Validate network address */
    if (!validate_ip_string(network)) {
        printf("Error: Invalid network address\n");
        return -1;
    }

    printf("Scanning subnet: %s/%d\n", network, prefix_len);
    
    int found = scan_subnet(network, prefix_len);

    printf("\n=== Results ===\n");
    printf("Found %d reachable host(s) in %s/%d\n\n", found, network, prefix_len);

    if (discovered_count > 0) {
        printf("%-18s %s\n", "IP Address", "Response Time");
        printf("%-18s %s\n", "----------", "-------------");
        
        for (int i = 0; i < discovered_count; i++) {
            printf("%-18s %d ms\n", 
                   discovered_hosts[i].ip_address,
                   discovered_hosts[i].response_time_ms);
        }
    }

    printf("\n");
    return found;
}

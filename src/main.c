/*
 * Network Monitoring and Visualization Tool
 * Main Program Entry Point
 * 
 * This is the main entry point for the network monitoring tool.
 * It provides a menu-based interface for monitoring Cisco network devices.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netmon.h"

/* Function prototypes */
static void display_menu(void);
static void handle_menu_choice(int choice);
static void view_statistics(void);
static void configure_devices(void);
static void discover_automatic_menu(void);
static void clear_screen(void);

int main(int argc, char *argv[])
{
    int choice;
    int running = 1;
    
    /* Suppress unused parameter warnings */
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("========================================\n");
    printf("  Network Monitoring & Visualization  \n");
    printf("  Tool for Cisco Networking Devices   \n");
    printf("========================================\n");
    printf("\n");
    
    /* Initialize the monitoring system */
    if (init_netmon() != 0) {
        fprintf(stderr, "Error: Failed to initialize network monitor\n");
        return EXIT_FAILURE;
    }
    
    /* Main program loop */
    while (running) {
        display_menu();
        
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            /* Clear invalid input */
            while (getchar() != '\n');
            printf("\nInvalid input. Please enter a number.\n");
            continue;
        }
        
        /* Clear input buffer */
        while (getchar() != '\n');
        
        if (choice == 0) {
            running = 0;
        } else {
            handle_menu_choice(choice);
        }
    }
    
    /* Cleanup and shutdown */
    cleanup_netmon();
    
    printf("\nThank you for using Network Monitor!\n");
    printf("Goodbye.\n\n");
    
    return EXIT_SUCCESS;
}

/*
 * Display the main menu
 */
static void display_menu(void)
{
    printf("\n");
    printf("=== Main Menu ===\n");
    printf("1. AUTOMATIC DISCOVERY (Discovers all hosts including other subnets)\n");
    printf("2. View Network Statistics\n");
    printf("3. Configure Devices\n");
    printf("0. Exit\n");
    printf("\n");
}

/*
 * Handle user menu selection
 */
static void handle_menu_choice(int choice)
{
    switch (choice) {
        case 1:
            discover_automatic_menu();
            break;
        case 2:
            view_statistics();
            break;
        case 3:
            configure_devices();
            break;
        default:
            printf("\nInvalid choice. Please select 0-3.\n");
            printf("Press Enter to continue...");
            getchar();
            break;
    }
}

/*
 * Discover network devices using ping
 */
/*
 * Automatic discovery menu - discovers everything without any input
 */
static void discover_automatic_menu(void)
{
    clear_screen();
    
    int found = discover_automatic();
    printf("\nAutomatic discovery complete. Found %d host(s).\n", found);
    
    printf("Press Enter to return to main menu...");
    getchar();
}

/*
 * View network statistics
 */
static void view_statistics(void)
{
    clear_screen();
    printf("\n=== Network Statistics ===\n\n");
    printf("Network Performance Metrics:\n");
    printf("- Total Devices: 0\n");
    printf("- Active Devices: 0\n");
    printf("- Average Response Time: N/A\n");
    printf("- Total Bandwidth: N/A\n");
    printf("\n(This feature is under development)\n\n");
    printf("Press Enter to return to main menu...");
    getchar();
}

/*
 * Configure network devices
 */
static void configure_devices(void)
{
    clear_screen();
    printf("\n=== Device Configuration ===\n\n");
    printf("Device configuration options:\n");
    printf("1. Add new device\n");
    printf("2. Remove device\n");
    printf("3. Edit device settings\n");
    printf("4. Load configuration file\n");
    printf("\n(This feature is under development)\n\n");
    printf("Press Enter to return to main menu...");
    getchar();
}

/*
 * Clear the screen (cross-platform)
 */
static void clear_screen(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

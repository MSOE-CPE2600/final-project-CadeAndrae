/**************************************************************
Filename: network_utils.c 
Description:
  Implements client-server communication for motion detection 
  using TCP sockets, with the server assigning tasks and the 
  client processing them.
Author: Cade Andrae
Date: 12/11/24
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "network_utils.h"
#include "handle_motion.h"
#include "main.h"

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to start the server
void start_server(const char* input_path, const char* output_path, int start_frame, int end_frame) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {                       // Create a socket for the server
        fprintf(stderr, "Error: Socket creation failed.\n");
        exit(EXIT_FAILURE);                                                         // Failed to create socket
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {   // Set socket options to allow reuse of the address
        fprintf(stderr, "Error: Setting socket options failed.\n");
        close(server_fd);                                                           // Close the socket
        exit(EXIT_FAILURE);                                                         // Exit the program
    }

    address.sin_family = AF_INET;                                                   // Use IPv4
    address.sin_addr.s_addr = INADDR_ANY;                                           // Accept connections from any IP
    address.sin_port = htons(PORT);                                                 // Set the port

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {         // Bind the socket to the address
        fprintf(stderr, "Error: Socket bind failed.\n");
        close(server_fd);                                                           // Close the socket
        exit(EXIT_FAILURE);                                                         // Exit the program
    }

    if (listen(server_fd, 1) < 0) {                                                 // Start listening for incoming connections
        fprintf(stderr, "Error: Socket listen failed.\n");
        close(server_fd);                                                           // Close the socket
        exit(EXIT_FAILURE);                                                         // Exit the program
    }
    printf("Server listening on port %d...\n", PORT);

    if ((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {        // Accept a client connection
        fprintf(stderr, "Error: Accepting client connection failed.\n");
        close(server_fd);                                                                                   // Close the socket
        exit(EXIT_FAILURE);                                                                                 // Exit the program
    }

    char buffer[BUFFER_SIZE] = {0};
    snprintf(buffer, sizeof(buffer), "PROCESS %s %s %d %d", input_path, output_path, start_frame, end_frame);
    send(client_socket, buffer, strlen(buffer), 0);                                 // Send processing command to the client
    printf("Server: Sent processing request to client.\n");

    memset(buffer, 0, BUFFER_SIZE);                                                 // Clear the buffer to prepare for receiving a response
    if (read(client_socket, buffer, BUFFER_SIZE) > 0) {                             // Check if data is received from the client
        printf("Server: Received confirmation from client: %s\n", buffer);
        if (strcmp(buffer, "COMPLETED") == 0) {                                     // Check if the client confirmed successful processing
            printf("Server: Client successfully completed the task.\n");
        } else {
            printf("Server: Unexpected message from client: %s\n", buffer);         // Handle invalid responses from the client
        }
    } else {
        fprintf(stderr, "Error: Failed to receive confirmation from client.\n");    // Print error when no response
    }
    
    close(client_socket);   // Close the client socket
    close(server_fd);       // Close the server socket
}

// Function to start the client
void start_client() {
    int client_socket;
    struct sockaddr_in server_address;

    while (1) {                                                                 // Loop to retry connection or return to main menu
        char user_input[16];
        printf("Client: Enter 'connect' to start or 'home' to cancel: ");
        scanf("%s", user_input);

        if (strcmp(user_input, "home") == 0) {                                  // Return to main menu if "home" is entered
            return;
        } else if (strcmp(user_input, "connect") != 0) {                        // Handle invalid input
            printf("Client: Invalid input. Please type 'connect' or 'home'.\n");
            continue;                                                           // Retry
        }

        if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {            // Create a socket for the client
            fprintf(stderr, "Error: Socket creation failed.\n");
            continue;                                                           // Retry
        }
        server_address.sin_family = AF_INET;                                    // Use IPv4
        server_address.sin_port = htons(PORT);                                  // Set the port
        
        if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {   // Convert the server address to binary form
            fprintf(stderr, "Error: Invalid address or address not supported.\n");
            close(client_socket);                                               // Close socket
            continue;                                                           // Retry
        }

        if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {    // Attempt to connect to the server
            fprintf(stderr, "Error: Unable to connect to the server. Retrying...\n");
            close(client_socket);                                                                       // Close socket
            continue;                                                                                   // Retry
        }

        printf("Client: Connected to server.\n");

        char buffer[BUFFER_SIZE] = {0};
        if (read(client_socket, buffer, BUFFER_SIZE) <= 0) {                                // Read the processing command from the server
            fprintf(stderr, "Error: Failed to receive processing command from server.\n");
            close(client_socket);                                                           // Close socket
            continue;                                                                       // Retry
        }
        printf("Client: Received processing command: %s\n", buffer);

        char input_path[MAX_PATH], output_path[MAX_PATH];
        int start_frame, end_frame;
        sscanf(buffer, "PROCESS %s %s %d %d", input_path, output_path, &start_frame, &end_frame);   // Parse the processing command
        printf("Client: Processing frames from %d to %d in directories:\n", start_frame, end_frame);
        printf(" - Input directory: %s\n", input_path);
        printf(" - Output directory: %s\n", output_path);

        process_frames_with_threads(input_path, output_path, end_frame + 1, start_frame);   // Process frames using the specified input and output paths

        snprintf(buffer, sizeof(buffer), "COMPLETED");                  // Notify the server of completion
        send(client_socket, buffer, strlen(buffer), 0);                 // Send confirmation
        printf("Client: Sent completion confirmation to server.\n");

        close(client_socket);                                           // Close the client socket
        break;                                                          // Exit after success
    }
}

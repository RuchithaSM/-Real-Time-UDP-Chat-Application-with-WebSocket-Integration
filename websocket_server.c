#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT_WS 5000
#define BUFFER_SIZE 1024

SOCKET server_socket;
struct sockaddr_in server_addr;
WSADATA wsa;

// Function to get the current timestamp
void get_timestamp(char *timestamp, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, size, "%Y-%m-%d %H:%M:%S", t);
}

// Function to handle client connections
DWORD WINAPI handle_client(LPVOID lpParam) {
    SOCKET client_socket = (SOCKET)lpParam;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while (1) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }

        buffer[bytes_received] = '\0';
        
        // Broadcast the message to all connected clients
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (i != client_socket) {
                send(i, buffer, bytes_received, 0);
            }
        }
    }

    closesocket(client_socket);
    return 0;
}

int main() {
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock\n");
        return 1;
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Failed to create socket\n");
        WSACleanup();
        return 1;
    }

    // Set up the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT_WS);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Failed to bind socket\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Failed to listen\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("WebSocket server started on port %d\n", PORT_WS);

    // Accept connections
    while (1) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            printf("Failed to accept connection\n");
            continue;
        }

        printf("New client connected\n");
        CreateThread(NULL, 0, handle_client, (LPVOID)client_socket, 0, NULL);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
} 
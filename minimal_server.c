#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 5000
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;
    char buffer[BUFFER_SIZE];
    int c;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code: %d", WSAGetLastError());
        return 1;
    }

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket: %d", WSAGetLastError());
        return 1;
    }

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Bind
    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed with error code: %d", WSAGetLastError());
        return 1;
    }

    // Listen
    listen(server_socket, 3);
    printf("Server started on port %d\n", PORT);

    // Accept connection
    c = sizeof(struct sockaddr_in);
    client_socket = accept(server_socket, (struct sockaddr*)&client, &c);
    if (client_socket == INVALID_SOCKET) {
        printf("Accept failed with error code: %d", WSAGetLastError());
        return 1;
    }

    printf("Client connected\n");

    // Receive and send data
    while (1) {
        int recv_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (recv_size > 0) {
            buffer[recv_size] = '\0';
            printf("Received: %s\n", buffer);
            send(client_socket, buffer, recv_size, 0);
        }
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
} 
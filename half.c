#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT_UDP 5000
#define BUFFER_SIZE 1024

SOCKET sock;
struct sockaddr_in server, client;
char nickname[30];

// Function to get the current timestamp
void get_timestamp(char *timestamp, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, size, "%Y-%m-%d %H:%M:%S", t);
}

// Function to send messages
DWORD WINAPI send_messages(LPVOID lpParam) {
    char buffer[BUFFER_SIZE];
    char packet[BUFFER_SIZE];
    char timestamp[30];

    printf("📢 Chat Mode Activated! Type messages below.\n");
    printf("🛑 Type '/exit' to quit.\n\n");

    while (1) {
        printf("%s: ", nickname);
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // Remove newline character

        // Check if user wants to exit
        if (strcmp(buffer, "/exit") == 0) {
            printf("🔴 Exiting chat...\n");
            exit(0);
        }

        get_timestamp(timestamp, sizeof(timestamp));

        snprintf(packet, BUFFER_SIZE,
                 "{ \"message\": \"%s\", \"sender\": \"%s\", \"receiver\": \"All\", \"timestamp\": \"%s\" }",
                 buffer, nickname, timestamp);

        sendto(sock, packet, strlen(packet), 0, (struct sockaddr *)&server, sizeof(server));
        printf("✅ Sent: %s\n", packet);
    }

    return 0;
}

// Function to receive messages
DWORD WINAPI receive_messages(LPVOID lpParam) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in from;
    int fromLen = sizeof(from);

    while (1) {
        int bytesReceived = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from, &fromLen);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            printf("\n📩 New Message: %s\n", buffer);
            printf("%s: ", nickname);
            fflush(stdout);
        }
    }
    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Set up the UDP server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT_UDP);

    // Bind socket to listen for incoming messages
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = INADDR_ANY;
    client.sin_port = htons(PORT_UDP);
    bind(sock, (struct sockaddr *)&client, sizeof(client));

    printf("Enter your nickname: ");
    fgets(nickname, 30, stdin);
    nickname[strcspn(nickname, "\n")] = 0;

    // Start sender and receiver threads
    CreateThread(NULL, 0, send_messages, NULL, 0, NULL);
    CreateThread(NULL, 0, receive_messages, NULL, 0, NULL);

    while (1) {}  // Keep the client running

    closesocket(sock);
    WSACleanup();
    return 0;
}

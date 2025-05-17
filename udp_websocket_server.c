#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <libwebsockets.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define UDP_PORT 5000
#define WS_PORT 9002
#define BUFFER_SIZE 1024

struct server_data {
    int udp_socket;
    struct sockaddr_in udp_client;
    struct lws *wsi;
};

// Structure to hold client information
struct per_session_data {
    struct lws *wsi;
    struct per_session_data *next;
};

// Global list of connected clients
static struct per_session_data *clients = NULL;

// WebSocket callback function
static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason,
                             void *user, void *in, size_t len) {
    struct per_session_data *pss = (struct per_session_data *)user;
    
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            printf("WebSocket Client Connected!\n");
            // Add client to the list
            pss = malloc(sizeof(struct per_session_data));
            pss->wsi = wsi;
            pss->next = clients;
            clients = pss;
            lws_set_wsi_user(wsi, pss);
            break;

        case LWS_CALLBACK_RECEIVE:
            printf("Received WebSocket Message: %.*s\n", (int)len, (char *)in);
            // Broadcast to all clients
            struct per_session_data *client = clients;
            while (client) {
                if (client->wsi != wsi) {  // Don't send back to sender
                    lws_write(client->wsi, (unsigned char *)in, len, LWS_WRITE_TEXT);
                }
                client = client->next;
            }
            break;

        case LWS_CALLBACK_CLOSED:
            printf("WebSocket Client Disconnected!\n");
            // Remove client from the list
            if (pss) {
                struct per_session_data **pp = &clients;
                while (*pp) {
                    if (*pp == pss) {
                        *pp = pss->next;
                        free(pss);
                        break;
                    }
                    pp = &(*pp)->next;
                }
            }
            break;

        default:
            break;
    }
    return 0;
}

// WebSocket protocol list
static const struct lws_protocols protocols[] = {
    {"http-only", websocket_callback, sizeof(struct per_session_data), BUFFER_SIZE},
    {NULL, NULL, 0, 0}
};

// WebSocket server thread
DWORD WINAPI websocket_server(LPVOID lpParam) {
    struct lws_context_creation_info info;
    struct lws_context *context;
    int n = 0;

    memset(&info, 0, sizeof info);
    info.port = WS_PORT;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "lws init failed\n");
        return 1;
    }

    printf("WebSocket server running on ws://localhost:%d\n", WS_PORT);

    while (n >= 0) {
        n = lws_service(context, 1000);
    }

    lws_context_destroy(context);
    return 0;
}

// UDP listener thread
DWORD WINAPI udp_listener(void *arg) {
    struct server_data *data = (struct server_data *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        int len = recvfrom(data->udp_socket, buffer, BUFFER_SIZE, 0,
                          (struct sockaddr *)&data->udp_client, &(int){sizeof(data->udp_client)});

        if (len > 0) {
            buffer[len] = '\0';
            printf("Received UDP Packet: %s\n", buffer);
            
            // Forward UDP message to all WebSocket clients
            struct per_session_data *client = clients;
            while (client) {
                lws_write(client->wsi, (unsigned char *)buffer, len, LWS_WRITE_TEXT);
                client = client->next;
            }
        }
    }

    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    struct server_data data;
    data.udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in udp_server = {0};
    udp_server.sin_family = AF_INET;
    udp_server.sin_addr.s_addr = INADDR_ANY;
    udp_server.sin_port = htons(UDP_PORT);

    bind(data.udp_socket, (struct sockaddr *)&udp_server, sizeof(udp_server));

    printf("UDP WebSocket Server is running...\n");

    HANDLE udpThread = CreateThread(NULL, 0, udp_listener, &data, 0, NULL);
    HANDLE wsThread = CreateThread(NULL, 0, websocket_server, NULL, 0, NULL);

    WaitForSingleObject(udpThread, INFINITE);
    WaitForSingleObject(wsThread, INFINITE);

    closesocket(data.udp_socket);
    WSACleanup();
    return 0;
}

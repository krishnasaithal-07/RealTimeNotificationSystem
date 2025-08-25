#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define PORT 12345
#define MAX_CLIENTS 10
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Target Windows 7 or later
#endif

SOCKET client_sockets[MAX_CLIENTS] = {0};
CRITICAL_SECTION lock;

void broadcast_message(const char *message, SOCKET sender_socket) {
    EnterCriticalSection(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0 && client_sockets[i] != sender_socket) {
            char msg_with_notification[1074];
            snprintf(msg_with_notification, sizeof(msg_with_notification), "Notification: %s", message);
            send(client_sockets[i], msg_with_notification, strlen(msg_with_notification), 0);
        }
    }
    LeaveCriticalSection(&lock);
}

DWORD WINAPI handle_client(LPVOID client_socket) {
    SOCKET socket = *(SOCKET *)client_socket;
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
            printf("Client disconnected\n");
            closesocket(socket);
            EnterCriticalSection(&lock);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == socket) {
                    client_sockets[i] = 0;
                    break;
                }
            }
            LeaveCriticalSection(&lock);
            break;
        }

        printf("Message received: %s", buffer);
        broadcast_message(buffer, socket);
    }

    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    InitializeCriticalSection(&lock);
    WSAStartup(MAKEWORD(2, 2), &wsa);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());

        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());

        closesocket(server_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    listen(server_socket, MAX_CLIENTS);
    printf("Server started on port %d\n", PORT);

    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket == INVALID_SOCKET) {
            printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());

            continue;
        }

        printf("New connection established\n");

        EnterCriticalSection(&lock);
        int added = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = new_socket;
                added = 1;
                break;
            }
        }
        LeaveCriticalSection(&lock);

        if (!added) {
            printf("Maximum clients reached. Connection rejected.\n");
            closesocket(new_socket);
            continue;
        }

        CreateThread(NULL, 0, handle_client, &new_socket, 0, NULL);
    }

    DeleteCriticalSection(&lock);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define PORT 12345
#define SERVER "127.0.0.1"  // Server IP
#define MAX_BUFFER_SIZE 1024

SOCKET connect_to_server() {
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server_addr;

    WSAStartup(MAKEWORD(2, 2), &wsa);

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER);
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        perror("Connection failed");
        closesocket(client_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    return client_socket;
}

void handle_server_response(SOCKET client_socket) {
    char buffer[MAX_BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            // Print the message from the server on a new line
            printf("\n%s\n", buffer);  // Message from server
            printf("Enter message: ");  // Display the prompt after message is received
        }
    }
}

int main() {
    SOCKET client_socket = connect_to_server();
    char message[MAX_BUFFER_SIZE];

    printf("Connected to the server. You can start typing your messages:\n");

    // Start a thread to handle server responses
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_server_response, (LPVOID)client_socket, 0, NULL);

    // Input loop for sending messages to the server
    while (1) {
        printf("Enter message: ");
        fgets(message, sizeof(message), stdin);
        send(client_socket, message, strlen(message), 0);
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
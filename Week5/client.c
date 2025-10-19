#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

void print_menu() {
    printf("\n=== MENU ===\n");
    printf("1. Dang nhap (login)\n");
    printf("2. Doi mat khau (change password)\n");
    printf("3. Xem homepage\n");
    printf("4. Thoat (bye)\n");
    printf("Lua chon cua ban: ");
}

int main(int argc, char *argv[]) {
    WSADATA wsa;
    SOCKET client_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE], response[BUFFER_SIZE];
    char server_ip[20];
    int server_port;
    int server_len = sizeof(server_addr);
    char logged_in_user[50] = "";
    int is_logged_in = 0;
    
    // Kiem tra tham so dong lenh
    if (argc != 3) {
        printf("Su dung: %s <IP_Address> <Port_Number>\n", argv[0]);
        printf("Vi du: %s 127.0.0.1 5500\n", argv[0]);
        return 1;
    }
    
    strcpy(server_ip, argv[1]);
    server_port = atoi(argv[2]);
    
    printf("=== UDP CLIENT ===\n");
    
    // Khoi tao Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Khoi tao Winsock that bai. Error: %d\n", WSAGetLastError());
        return 1;
    }
    
    // Tao socket
    client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_sock == INVALID_SOCKET) {
        printf("Khong the tao socket. Error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    // Cau hinh dia chi server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    printf("Ket noi den server %s:%d\n", server_ip, server_port);
    
    while (1) {
        print_menu();
        int choice;
        scanf("%d", &choice);
        getchar(); // Loai bo newline
        
        memset(buffer, 0, BUFFER_SIZE);
        memset(response, 0, BUFFER_SIZE);
        
        switch (choice) {
            case 1: { // Dang nhap
                char username[50], password[50];
                printf("Nhap username: ");
                scanf("%s", username);
                printf("Nhap password: ");
                scanf("%s", password);
                
                sprintf(buffer, "login %s %s", username, password);
                
                if (sendto(client_sock, buffer, strlen(buffer), 0,
                          (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
                    printf("sendto() that bai. Error: %d\n", WSAGetLastError());
                    continue;
                }
                
                int recv_len = recvfrom(client_sock, response, BUFFER_SIZE, 0,
                                       (struct sockaddr*)&server_addr, &server_len);
                
                if (recv_len == SOCKET_ERROR) {
                    printf("recvfrom() that bai. Error: %d\n", WSAGetLastError());
                    continue;
                }
                
                printf("Server tra ve: %s\n", response);
                
                if (strcmp(response, "OK") == 0) {
                    strcpy(logged_in_user, username);
                    is_logged_in = 1;
                    printf("Dang nhap thanh cong!\n");
                } else if (strcmp(response, "Account is blocked") == 0) {
                    printf("Tai khoan da bi khoa!\n");
                } else if (strcmp(response, "account not ready") == 0) {
                    printf("Tai khoan khong ton tai!\n");
                } else {
                    printf("Sai mat khau!\n");
                }
                break;
            }
            
            case 2: { // Doi mat khau
                if (!is_logged_in) {
                    printf("Ban chua dang nhap!\n");
                    break;
                }
                
                char new_password[50];
                printf("Nhap mat khau moi: ");
                scanf("%s", new_password);
                
                sprintf(buffer, "change %s %s", logged_in_user, new_password);
                
                if (sendto(client_sock, buffer, strlen(buffer), 0,
                          (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
                    printf("sendto() that bai. Error: %d\n", WSAGetLastError());
                    continue;
                }
                
                int recv_len = recvfrom(client_sock, response, BUFFER_SIZE, 0,
                                       (struct sockaddr*)&server_addr, &server_len);
                
                if (recv_len == SOCKET_ERROR) {
                    printf("recvfrom() that bai. Error: %d\n", WSAGetLastError());
                    continue;
                }
                
                printf("Server tra ve: %s\n", response);
                
                if (strcmp(response, "OK") == 0) {
                    printf("Doi mat khau thanh cong!\n");
                } else {
                    printf("Mat khau khong hop le! (Can co ca chu va so)\n");
                }
                break;
            }
            
            case 3: { // Xem homepage
                if (!is_logged_in) {
                    printf("Ban chua dang nhap!\n");
                    break;
                }
                
                sprintf(buffer, "homepage %s", logged_in_user);
                
                if (sendto(client_sock, buffer, strlen(buffer), 0,
                          (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
                    printf("sendto() that bai. Error: %d\n", WSAGetLastError());
                    continue;
                }
                
                int recv_len = recvfrom(client_sock, response, BUFFER_SIZE, 0,
                                       (struct sockaddr*)&server_addr, &server_len);
                
                if (recv_len == SOCKET_ERROR) {
                    printf("recvfrom() that bai. Error: %d\n", WSAGetLastError());
                    continue;
                }
                
                printf("Homepage: %s\n", response);
                break;
            }
            
            case 4: { // Thoat
                sprintf(buffer, "bye");
                
                if (sendto(client_sock, buffer, strlen(buffer), 0,
                          (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
                    printf("sendto() that bai. Error: %d\n", WSAGetLastError());
                }
                
                int recv_len = recvfrom(client_sock, response, BUFFER_SIZE, 0,
                                       (struct sockaddr*)&server_addr, &server_len);
                
                if (recv_len > 0) {
                    printf("Server tra ve: %s\n", response);
                }
                
                printf("Tam biet!\n");
                closesocket(client_sock);
                WSACleanup();
                return 0;
            }
            
            default:
                printf("Lua chon khong hop le!\n");
                break;
        }
    }
    
    closesocket(client_sock);
    WSACleanup();
    return 0;
}
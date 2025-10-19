#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 5500
#define BUFFER_SIZE 1024
#define MAX_ACCOUNTS 100
#define MAX_ATTEMPTS 3

typedef struct {
    char username[50];
    char password[50];
    char email[100];
    char homepage[100];
    int status; // 1: active, 0: blocked
    int failed_attempts;
} Account;

Account accounts[MAX_ACCOUNTS];
int account_count = 0;

void load_accounts() {
    FILE *fp = fopen("account.txt", "r");
    if (fp == NULL) {
        printf("Khong the mo file account.txt\n");
        return;
    }
    
    while (fscanf(fp, "%s %s %s %s %d", 
                  accounts[account_count].username,
                  accounts[account_count].password,
                  accounts[account_count].email,
                  accounts[account_count].homepage,
                  &accounts[account_count].status) == 5) {
        accounts[account_count].failed_attempts = 0;
        account_count++;
    }
    
    fclose(fp);
    printf("Da tai %d tai khoan tu file\n", account_count);
}

void save_accounts() {
    FILE *fp = fopen("account.txt", "w");
    if (fp == NULL) {
        printf("Khong the ghi file account.txt\n");
        return;
    }
    
    for (int i = 0; i < account_count; i++) {
        fprintf(fp, "%s %s %s %s %d\n",
                accounts[i].username,
                accounts[i].password,
                accounts[i].email,
                accounts[i].homepage,
                accounts[i].status);
    }
    
    fclose(fp);
}

int find_account(const char *username) {
    for (int i = 0; i < account_count; i++) {
        if (strcmp(accounts[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

int is_valid_password(const char *password) {
    int has_letter = 0, has_digit = 0;
    for (int i = 0; password[i]; i++) {
        if ((password[i] >= 'a' && password[i] <= 'z') || 
            (password[i] >= 'A' && password[i] <= 'Z')) {
            has_letter = 1;
        }
        if (password[i] >= '0' && password[i] <= '9') {
            has_digit = 1;
        }
    }
    return has_letter && has_digit;
}

void process_request(char *buffer, char *response, struct sockaddr_in *client_addr) {
    char cmd[20], username[50], password[50], new_password[50];
    
    if (sscanf(buffer, "%s", cmd) != 1) {
        strcpy(response, "Error");
        return;
    }
    
    if (strcmp(cmd, "login") == 0) {
        if (sscanf(buffer, "%s %s %s", cmd, username, password) != 3) {
            strcpy(response, "Error");
            return;
        }
        
        int idx = find_account(username);
        if (idx == -1) {
            strcpy(response, "account not ready");
            return;
        }
        
        if (accounts[idx].status == 0) {
            strcpy(response, "Account is blocked");
            return;
        }
        
        if (strcmp(accounts[idx].password, password) == 0) {
            accounts[idx].failed_attempts = 0;
            strcpy(response, "OK");
        } else {
            accounts[idx].failed_attempts++;
            if (accounts[idx].failed_attempts >= MAX_ATTEMPTS) {
                accounts[idx].status = 0;
                save_accounts();
                strcpy(response, "Account is blocked");
            } else {
                strcpy(response, "not OK");
            }
        }
    }
    else if (strcmp(cmd, "bye") == 0) {
        strcpy(response, "Goodbye");
    }
    else if (strcmp(cmd, "change") == 0) {
        if (sscanf(buffer, "%s %s %s", cmd, username, new_password) != 3) {
            strcpy(response, "Error");
            return;
        }
        
        int idx = find_account(username);
        if (idx == -1) {
            strcpy(response, "Error");
            return;
        }
        
        if (!is_valid_password(new_password)) {
            strcpy(response, "Error");
            return;
        }
        
        strcpy(accounts[idx].password, new_password);
        save_accounts();
        strcpy(response, "OK");
    }
    else if (strcmp(cmd, "homepage") == 0) {
        if (sscanf(buffer, "%s %s", cmd, username) != 2) {
            strcpy(response, "Error");
            return;
        }
        
        int idx = find_account(username);
        if (idx == -1) {
            strcpy(response, "Error");
            return;
        }
        
        strcpy(response, accounts[idx].homepage);
    }
    else {
        strcpy(response, "Error");
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE], response[BUFFER_SIZE];
    int client_len = sizeof(client_addr);
    
    printf("=== UDP SERVER ===\n");
    
    // Khoi tao Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Khoi tao Winsock that bai. Error: %d\n", WSAGetLastError());
        return 1;
    }
    
    // Tai tai khoan tu file
    load_accounts();
    
    // Tao socket
    server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_sock == INVALID_SOCKET) {
        printf("Khong the tao socket. Error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    // Cau hinh dia chi server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind that bai. Error: %d\n", WSAGetLastError());
        closesocket(server_sock);
        WSACleanup();
        return 1;
    }
    
    printf("Server dang lang nghe tren cong %d...\n", PORT);
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        memset(response, 0, BUFFER_SIZE);
        
        int recv_len = recvfrom(server_sock, buffer, BUFFER_SIZE, 0, 
                               (struct sockaddr*)&client_addr, &client_len);
        
        if (recv_len == SOCKET_ERROR) {
            printf("recvfrom() that bai. Error: %d\n", WSAGetLastError());
            continue;
        }
        
        printf("Nhan tu client: %s\n", buffer);
        
        process_request(buffer, response, &client_addr);
        
        printf("Gui den client: %s\n", response);
        
        if (sendto(server_sock, response, strlen(response), 0,
                  (struct sockaddr*)&client_addr, client_len) == SOCKET_ERROR) {
            printf("sendto() that bai. Error: %d\n", WSAGetLastError());
        }
    }
    
    closesocket(server_sock);
    WSACleanup();
    return 0;
}
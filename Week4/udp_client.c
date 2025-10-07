#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
#else
    #include <unistd.h>
    #include <arpa/inet.h>
#endif

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

int main() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif

    int sockfd;
    char buffer[BUFFER_SIZE];
    char *message = "Hello world";
    struct sockaddr_in servaddr;
    
    // Tạo socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    
    // Cấu hình địa chỉ server
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Gửi thông điệp đến server
    printf("Gui thong diep: %s\n", message);
    sendto(sockfd, message, strlen(message), 0,
          (const struct sockaddr *)&servaddr, sizeof(servaddr));
    
    // Nhận phản hồi từ server
    memset(buffer, 0, BUFFER_SIZE);
    int len = sizeof(servaddr);
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                    (struct sockaddr *)&servaddr, &len);
    
    if (n < 0) {
        perror("Receive failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    buffer[n] = '\0';
    
    printf("Nhan phan hoi tu server: %s\n", buffer);
    
    close(sockfd);
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}
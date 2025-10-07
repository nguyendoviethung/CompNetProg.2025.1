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
    struct sockaddr_in servaddr, cliaddr;
    int len;
    
    // Tạo socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    
    // Cấu hình địa chỉ server
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    
    // Bind socket với địa chỉ
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("UDP Server dang lang nghe tren port %d...\n", PORT);
    
    len = sizeof(cliaddr);
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        
        // Nhận dữ liệu từ client
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, 
                        (struct sockaddr *)&cliaddr, &len);
        
        if (n < 0) {
            perror("Receive failed");
            continue;
        }
        
        buffer[n] = '\0';
        
        // Lấy thông tin client
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cliaddr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(cliaddr.sin_port);
        
        // Lấy thông tin server
        struct sockaddr_in server_info;
        int server_len = sizeof(server_info);
        getsockname(sockfd, (struct sockaddr *)&server_info, &server_len);
        char server_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(server_info.sin_addr), server_ip, INET_ADDRSTRLEN);
        int server_port = ntohs(server_info.sin_port);
        
        printf("Nhan duoc tu client: %s\n", buffer);
        printf("Client: %s:%d\n", client_ip, client_port);
        
        // Tạo thông điệp phản hồi
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE, 
                "Hello %s:%d from %s:%d", 
                client_ip, client_port, server_ip, server_port);
        
        // Gửi phản hồi cho client
        sendto(sockfd, response, strlen(response), 0,
              (const struct sockaddr *)&cliaddr, len);
        
        printf("Da gui phan hoi: %s\n\n", response);
    }
    
    close(sockfd);
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

// ---- Nhận diện chuỗi trông giống IP để báo "Invalid address" khi không parse được ----
static int looks_like_ipv4(const char *s) {
    int dots = 0, digits = 0;
    if (!s || !*s) return 0;
    for (const char *p = s; *p; ++p) {
        if (*p == '.') { dots++; digits = 0; }
        else if (isdigit((unsigned char)*p)) { digits++; if (digits > 3) return 0; }
        else return 0;
    }
    return dots >= 1;
}
static int looks_like_ipv6(const char *s) {
    int colons = 0;
    if (!s || !*s) return 0;
    for (const char *p = s; *p; ++p) {
        if (*p == ':') colons++;
        else if (isxdigit((unsigned char)*p) || *p == '.' || *p == '%') continue;
        else return 0;
    }
    return colons >= 2;
}
static int looks_like_ip(const char *s) { return looks_like_ipv4(s) || looks_like_ipv6(s); }

// ---- In danh sách trên một dòng sau nhãn (Alias IP / Alias name) ----
static void print_list_with_label(const char *label, char **items, size_t n) {
    if (!items || n == 0) return;
    printf("%s", label);
    for (size_t i = 0; i < n; ++i) {
        printf("%s%s", items[i], (i + 1 < n) ? " " : "");
    }
    putchar('\n');
}

// ---- Thu thập IP (unique) từ addrinfo ----
typedef struct { char **data; size_t len, cap; } strlist;
static void sl_init(strlist *sl){ sl->data=NULL; sl->len=sl->cap=0; }
static void sl_free(strlist *sl){ for(size_t i=0;i<sl->len;i++) free(sl->data[i]); free(sl->data); }
static int sl_has(strlist *sl, const char *s){ for(size_t i=0;i<sl->len;i++) if(strcmp(sl->data[i],s)==0) return 1; return 0; }
static void sl_push_unique(strlist *sl, const char *s){
    if(!s || sl_has(sl,s)) return;
    if(sl->len==sl->cap){ size_t nc=sl->cap?sl->cap*2:8; sl->data=realloc(sl->data,nc*sizeof(char*)); sl->cap=nc; }
    sl->data[sl->len++]=strdup(s);
}

// ---- Forward lookup: domain -> IPs ----
static int handle_domain(const char *domain){
    struct addrinfo hints, *res=NULL;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;       // IPv4 + IPv6
    hints.ai_socktype = SOCK_STREAM;   // tránh trùng lặp nhiều bản ghi
    hints.ai_flags = AI_CANONNAME;
    int rc = getaddrinfo(domain,NULL,&hints,&res);
    if(rc!=0 || !res){
        puts("Not found information");
        if(res) freeaddrinfo(res);
        return -1;
    }

    strlist ips; sl_init(&ips);
    for(struct addrinfo *p=res;p;p=p->ai_next){
        char ip[NI_MAXHOST];
        if(getnameinfo(p->ai_addr,(socklen_t)((p->ai_family==AF_INET)?sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6)),
                       ip,sizeof(ip),NULL,0,NI_NUMERICHOST)==0){
            sl_push_unique(&ips, ip);
        }
    }

    if(ips.len==0){
        puts("Not found information");
    }else{
        printf("Official IP: %s\n", ips.data[0]);
        if(ips.len>1){
            print_list_with_label("Alias IP: ", ips.data+1, ips.len-1);
        }
    }
    sl_free(&ips);
    freeaddrinfo(res);
    return 0;
}

// ---- Reverse lookup: IP -> official name + aliases ----
static int handle_ip_numeric(const char *ip){
    // Dùng AI_NUMERICHOST để xác thực IP
    struct addrinfo hints,*num=NULL;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags  = AI_NUMERICHOST;
    if(getaddrinfo(ip,NULL,&hints,&num)!=0 || !num){
        puts("Invalid address");
        if(num) freeaddrinfo(num);
        return -1;
    }

    // Official name
    char host[NI_MAXHOST];
    int rc = getnameinfo(num->ai_addr,(socklen_t)num->ai_addrlen,host,sizeof(host),NULL,0,NI_NAMEREQD);
    if(rc!=0){
        // Không có PTR -> coi như không tìm thấy
        puts("Not found information");
        freeaddrinfo(num);
        return -1;
    }
    printf("Official name: %s\n", host);

    // Alias name (nếu có) qua gethostbyaddr
    const void *addrptr=NULL; socklen_t addrlen=0; int family=num->ai_family;
    if(family==AF_INET){ addrptr=&((struct sockaddr_in*)num->ai_addr)->sin_addr; addrlen=sizeof(struct in_addr); }
    else { addrptr=&((struct sockaddr_in6*)num->ai_addr)->sin6_addr; addrlen=sizeof(struct in6_addr); }

    struct hostent *he = gethostbyaddr(addrptr, addrlen, family);
    if(he && he->h_aliases && he->h_aliases[0]){
        // gom alias
        size_t count=0; while(he->h_aliases[count]) count++;
        print_list_with_label("Alias name: ", he->h_aliases, count);
    }

    freeaddrinfo(num);
    return 0;
}

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <domain-or-ip>\n", argv[0]);
        return 1;
    }
    const char *param = argv[1];

    // Thử parse như IP số trước (để xác thực "Invalid address")
    struct addrinfo hints,*num=NULL;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags  = AI_NUMERICHOST;
    int is_numeric_ok = (getaddrinfo(param,NULL,&hints,&num) == 0) && num != NULL;
    if(is_numeric_ok){
        freeaddrinfo(num);
        return handle_ip_numeric(param)==0 ? 0 : 2;
    }

    // Nếu trông giống IP nhưng không parse được -> Invalid address
    if(looks_like_ip(param)){
        puts("Invalid address");
        if(num) freeaddrinfo(num);
        return 2;
    }

    // Ngược lại, coi là tên miền
    if(num) freeaddrinfo(num);
    return handle_domain(param)==0 ? 0 : 3;
}

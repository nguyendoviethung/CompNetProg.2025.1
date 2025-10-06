#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOG_FILE "resolver.log"

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

// ---- Tiện ích chuỗi ----
static char *trim(char *s) {
    if (!s) return s;
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
    size_t i = 0;
    while (isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
    n = strlen(s);
    while (n && isspace((unsigned char)s[n-1])) s[--n] = '\0';
    return s;
}

static int looks_like_ipv4(const char *s) {
    if (!s) return 0;
    int dots = 0, digits = 0;
    for (const char *p = s; *p; ++p) {
        if (*p == '.') { dots++; digits = 0; }
        else if (isdigit((unsigned char)*p)) { digits++; if (digits > 3) return 0; }
        else return 0;
    }
    return dots >= 1;
}
static int looks_like_ipv6(const char *s) {
    if (!s) return 0;
    int colons = 0;
    for (const char *p = s; *p; ++p) {
        if (*p == ':') colons++;
        else if (isxdigit((unsigned char)*p) || *p == '.' || *p == '%') continue;
        else return 0;
    }
    return colons >= 2;
}
static int looks_like_ip(const char *s) { return looks_like_ipv4(s) || looks_like_ipv6(s); }

static void now_iso(char *buf, size_t sz) {
    time_t t = time(NULL);
    struct tm tm; localtime_r(&t, &tm);
    strftime(buf, sz, "%Y-%m-%d %H:%M:%S", &tm);
}
static double now_millis(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

// ---- List chuỗi ----
typedef struct { char **data; size_t len, cap; } strlist;
static void sl_init(strlist *sl){ sl->data=NULL; sl->len=sl->cap=0; }
static void sl_free(strlist *sl){ for(size_t i=0;i<sl->len;i++) free(sl->data[i]); free(sl->data); }
static int sl_has(const strlist *sl, const char *s){ for(size_t i=0;i<sl->len;i++) if(strcmp(sl->data[i],s)==0) return 1; return 0; }
static void sl_push_unique(strlist *sl, const char *s){
    if(!s || sl_has(sl,s)) return;
    if(sl->len==sl->cap){ size_t nc=sl->cap?sl->cap*2:8; sl->data=realloc(sl->data,nc*sizeof(char*)); sl->cap=nc; }
    sl->data[sl->len++]=strdup(s);
}

// ---- In IP số từ sockaddr ----
static int sockaddr_to_ipstr(const struct sockaddr *sa, char *buf, size_t buflen) {
    return getnameinfo(sa, (socklen_t)((sa->sa_family==AF_INET)?sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6)),
                       buf, (socklen_t)buflen, NULL, 0, NI_NUMERICHOST);
}

// ---- Phân loại IP đặc biệt ----
static int is_private_ipv4(uint32_t host) {
    if ((host & 0xFF000000u) == 0x0A000000u) return 1;        // 10.0.0.0/8
    if ((host & 0xFFF00000u) == 0xAC100000u) return 1;        // 172.16.0.0/12
    if ((host & 0xFFFF0000u) == 0xC0A80000u) return 1;        // 192.168.0.0/16
    return 0;
}
static const char* classify_special_ip(const struct sockaddr *sa) {
    static char type[64]; type[0]='\0';
    if (!sa) return NULL;
    if (sa->sa_family == AF_INET) {
        const struct sockaddr_in *sin = (const struct sockaddr_in*)sa;
        uint32_t host = ntohl(sin->sin_addr.s_addr);
        if ((host & 0xFF000000u) == 0x7F000000u) { strcpy(type,"loopback"); return type; }
        if (is_private_ipv4(host)) { strcpy(type,"private"); return type; }
        if ((host & 0xFFFF0000u) == 0xA9FE0000u) { strcpy(type,"link-local"); return type; }
        if ((host & 0xF0000000u) == 0xE0000000u) { strcpy(type,"multicast"); return type; }
        if (host == 0) { strcpy(type,"unspecified"); return type; }
    } else if (sa->sa_family == AF_INET6) {
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6*)sa;
        const unsigned char *a = sin6->sin6_addr.s6_addr;
        static const unsigned char loopback[16] = { [15]=1 };
        static const unsigned char unspecified[16] = {0};
        if (memcmp(a, loopback, 16) == 0) { strcpy(type,"loopback"); return type; }
        if (memcmp(a, unspecified, 16) == 0) { strcpy(type,"unspecified"); return type; }
        if (a[0] == 0xFF) { strcpy(type,"multicast"); return type; }
        if (a[0] == 0xFE && (a[1] & 0xC0) == 0x80) { strcpy(type,"link-local"); return type; }
        if ((a[0] & 0xFE) == 0xFC) { strcpy(type,"unique-local"); return type; }
    }
    return NULL;
}

// ---- Log ----
static void log_line(FILE *fp, const char *line) {
    if (!fp || !line) return;
    fputs(line, fp);
    if (line[strlen(line)-1] != '\n') fputc('\n', fp);
    fflush(fp);
}

// ---- In theo format BT1 ----
static void print_bt1_forward(const strlist *ips, FILE *logfp) {
    if (!ips || ips->len == 0) {
        puts("Not found information");
        log_line(logfp, "Not found information");
        return;
    }
    printf("Official IP: %s\n", ips->data[0]);
    char buf[1024]; snprintf(buf, sizeof(buf), "Official IP: %s", ips->data[0]); log_line(logfp, buf);
    if (ips->len > 1) {
        printf("Alias IP: ");
        strcpy(buf, "Alias IP: ");
        for (size_t i=1;i<ips->len;i++) {
            printf("%s%s", ips->data[i], (i+1<ips->len) ? " " : "");
            strcat(buf, ips->data[i]); if (i+1<ips->len) strcat(buf, " ");
        }
        putchar('\n'); log_line(logfp, buf);
    }
}
static void print_bt1_reverse(const char *official_name, const strlist *aliases, FILE *logfp) {
    if (!official_name || !*official_name) {
        puts("Not found information"); log_line(logfp, "Not found information"); return;
    }
    char buf[2048];
    printf("Official name: %s\n", official_name);
    snprintf(buf, sizeof(buf), "Official name: %s", official_name); log_line(logfp, buf);
    if (aliases && aliases->len > 0) {
        printf("Alias name: ");
        strcpy(buf, "Alias name: ");
        for (size_t i=0;i<aliases->len;i++) {
            printf("%s%s", aliases->data[i], (i+1<aliases->len) ? " " : "");
            strcat(buf, aliases->data[i]); if (i+1<aliases->len) strcat(buf, " ");
        }
        putchar('\n'); log_line(logfp, buf);
    }
}

// ---- Forward lookup ----
static int forward_lookup(const char *host, int simple_mode, FILE *logfp) {
    double t0 = now_millis();
    struct addrinfo hints; memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    struct addrinfo *res=NULL;
    int rc = getaddrinfo(host,NULL,&hints,&res);
    if (rc != 0) {
        if (simple_mode) { puts("Not found information"); log_line(logfp, "Not found information"); }
        else {
            printf("[Query: %s] Not found information (getaddrinfo: %s)\n", host, gai_strerror(rc));
            char b[512]; snprintf(b,sizeof(b),"[Query: %s] Not found information (getaddrinfo: %s)", host, gai_strerror(rc));
            log_line(logfp, b);
        }
        return -1;
    }
    strlist ips; sl_init(&ips);
    const char *canon=NULL;
    for (struct addrinfo *p=res; p; p=p->ai_next) {
        char ip[NI_MAXHOST];
        if (sockaddr_to_ipstr(p->ai_addr, ip, sizeof(ip))==0) sl_push_unique(&ips, ip);
        if (!canon && p->ai_canonname) canon = p->ai_canonname;
    }

    if (simple_mode) {
        print_bt1_forward(&ips, logfp);
    } else {
        if (ips.len==0) {
            printf("[Query: %s] Not found information\n", host);
            log_line(logfp, "[No IP addresses]");
        } else {
            print_bt1_forward(&ips, logfp);
            if (canon && *canon) {
                printf("CNAME (canonical name): %s\n", canon);
                char l2[1400]; snprintf(l2,sizeof(l2),"CNAME: %s", canon); log_line(logfp,l2);
            }
        }
        double t1 = now_millis();
        printf("Query time: %.3f ms\n", t1 - t0);
        char lq[256]; snprintf(lq,sizeof(lq),"Query time: %.3f ms", t1 - t0); log_line(logfp, lq);
    }
    freeaddrinfo(res); sl_free(&ips);
    return 0;
}

// ---- Reverse lookup ----
static int reverse_lookup(const struct addrinfo *numeric, int simple_mode, FILE *logfp) {
    double t0 = now_millis();

    char host[NI_MAXHOST];
    int rc = getnameinfo(numeric->ai_addr, (socklen_t)numeric->ai_addrlen, host, sizeof(host), NULL, 0, NI_NAMEREQD);
    const char *official = (rc==0) ? host : NULL;

    strlist aliases; sl_init(&aliases);
    if (numeric->ai_family==AF_INET || numeric->ai_family==AF_INET6) {
        const void *addrptr=NULL; socklen_t addrlen=0; int family=numeric->ai_family;
        if (family==AF_INET){ addrptr=&((struct sockaddr_in*)numeric->ai_addr)->sin_addr; addrlen=sizeof(struct in_addr); }
        else { addrptr=&((struct sockaddr_in6*)numeric->ai_addr)->sin6_addr; addrlen=sizeof(struct in6_addr); }
        struct hostent *he = gethostbyaddr(addrptr, addrlen, family);
        if (he && he->h_aliases) for (char **pp=he->h_aliases; *pp; ++pp) sl_push_unique(&aliases, *pp);
    }

    const char *special = classify_special_ip(numeric->ai_addr);
    if (simple_mode) {
        print_bt1_reverse(official, &aliases, logfp);
    } else {
        if (special) { puts("special IP address — may not have DNS record"); log_line(logfp, "special IP address — may not have DNS record"); }
        if (!official) { puts("Not found information"); log_line(logfp, "Not found information"); }
        else { print_bt1_reverse(official, &aliases, logfp); }
        double t1 = now_millis();
        printf("Query time: %.3f ms\n", t1 - t0);
        char lq[256]; snprintf(lq,sizeof(lq),"Query time: %.3f ms", t1 - t0); log_line(logfp, lq);
    }

    sl_free(&aliases);
    return (official!=NULL) ? 0 : -1;
}

// ---- Xử lý 1 token ----
static void process_one(const char *token, int simple_mode, FILE *logfp) {
    if (!token || !*token) return;
    char ts[32]; now_iso(ts,sizeof(ts));
    char head[2048]; snprintf(head,sizeof(head),"----- [%s] Query: %s -----", ts, token);
    log_line(logfp, head);

    // Thử IP số
    struct addrinfo hints,*num=NULL; memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC; hints.ai_flags = AI_NUMERICHOST;
    int rc = getaddrinfo(token,NULL,&hints,&num);
    if (rc==0 && num) {
        reverse_lookup(num, simple_mode, logfp);
        freeaddrinfo(num);
        return;
    }
    if (looks_like_ip(token)) { puts("Invalid address"); log_line(logfp,"Invalid address"); if (num) freeaddrinfo(num); return; }
    forward_lookup(token, simple_mode, logfp);
    if (num) freeaddrinfo(num);
}

// ---- Batch mode ----
static int is_regular_file(const char *path) {
    struct stat st; if (stat(path, &st)!=0) return 0; return S_ISREG(st.st_mode);
}
static void batch_mode(const char *filepath) {
    FILE *fp = fopen(filepath,"r"); if (!fp) { perror("open list file"); return; }
    FILE *logfp = fopen(LOG_FILE,"a"); if (!logfp) { perror("open log file"); logfp = stdout; }
    char *line=NULL; size_t cap=0; while (1) {
        ssize_t n = getline(&line,&cap,fp); if (n<0) break;
        trim(line); if (!*line || line[0]=='#') continue;
        char *save=NULL; for (char *tok=strtok_r(line," \t",&save); tok; tok=strtok_r(NULL," \t",&save)) process_one(tok, 0, logfp);
    }
    free(line); fclose(fp); if (logfp && logfp!=stdout) fclose(logfp);
}

// ---- Interactive ----
static void interactive_loop(void) {
    FILE *logfp = fopen(LOG_FILE,"a"); if (!logfp) { perror("open log file"); logfp = stdout; }
    puts("=== Resolver (BT2) — nhập IP/tên miền; nhiều token trên 1 dòng; Enter trống để thoát ===");
    char *line=NULL; size_t cap=0;
    for (;;) {
        printf("> "); fflush(stdout);
        ssize_t n = getline(&line,&cap,stdin); if (n<0) break;
        trim(line); if (!*line) break;
        char *save=NULL; for (char *tok=strtok_r(line," \t",&save); tok; tok=strtok_r(NULL," \t",&save)) process_one(tok, 0, logfp);
    }
    free(line); if (logfp && logfp!=stdout) fclose(logfp);
}

// ---- main ----
static void print_usage(const char *p){
    fprintf(stderr,"Usage:\n  %s <parameter|listfile>\n  %s  (no args: interactive)\n", p, p);
}

int main(int argc, char **argv) {
    if (argc == 1) { interactive_loop(); return 0; }

    const char *param = argv[1];
    // Nếu mở được như file thường -> batch mode
    if (is_regular_file(param)) { batch_mode(param); return 0; }

    // BT1 single-parameter output style (để tương thích)
    FILE *logfp = fopen(LOG_FILE,"a"); if (!logfp) { perror("open log file"); logfp = stdout; }
    process_one(param, /*simple_mode=*/1, logfp);
    if (logfp && logfp!=stdout) fclose(logfp);
    return 0;
}

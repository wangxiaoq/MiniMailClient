#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define MAX_BUF 4096
#define NAME_LEN 512

#define PORT 25

#define FATAL(s) do { \
    perror(s); \
    exit(-1); \
} while (0)

int create_tcp_server(char *addr)
{
    struct sockaddr_in in_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
    };
    in_addr.sin_addr.s_addr = inet_addr(addr);

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        return sock_fd;
    }

    if (connect(sock_fd, (struct sockaddr*)&in_addr, sizeof(in_addr)) < 0) {
        return -1;
    }

    return sock_fd;
}

static void recv_and_check(int sock_fd)
{
    char recv_buf[MAX_BUF] = {0};
    int status_code = 0;
    char line[MAX_BUF] = {0};
    int ret = 0;

    ret = recv(sock_fd, recv_buf, MAX_BUF, 0);
    sscanf(recv_buf, "%d%s", &status_code, line);
    if (status_code != 250 && status_code != 220 && status_code != 354 && status_code != 221) {
        FATAL(line);
    }
}

void input_content(char *sender, char *recipient, char *mail_content)
{
    printf("MAIL FROM: ");
    scanf("%s", sender);

    printf("RCPT TO: ");
    scanf("%s", recipient);

    printf("MAIL CONTENT: ");
    scanf("%s", mail_content);
    strcat(mail_content, "\r\n");
}

void get_server_name(char *mail, char *name)
{
    char user[NAME_LEN] = {0};

    sscanf(mail, "%[^@]%*c%s", user, name);
    sprintf(user, "mail.%s", name);
    strcpy(name, user);
}

void get_recipient_server_ip(char *name, char *ip)
{
    struct hostent *host;
    struct in_addr addr;

    host = gethostbyname(name);
    if (host == NULL) {
        printf("Cannot acquire recipient server IP, please input:");
        scanf("%s", ip);
        return ;
    }

    memcpy(&addr.s_addr,host->h_addr,4);
    strcpy(ip, inet_ntoa(addr));
}

int main(void)
{
    char sender[NAME_LEN] = {0};
    char sender_server[NAME_LEN] = {0};
    char recipient[NAME_LEN] = {0};
    char recipient_server_name[NAME_LEN] = {0};
    char recipient_server_ip[NAME_LEN] = {0};
    char mail_content[MAX_BUF] = {0};
    char line[MAX_BUF] = {0};
    int sock_fd = -1;
    int ret = 0;

    input_content(sender, recipient, mail_content);
    get_server_name(recipient, recipient_server_name);
    get_recipient_server_ip(recipient_server_name, recipient_server_ip);
    get_server_name(sender, sender_server);

    sock_fd = create_tcp_server(recipient_server_ip);
    if (sock_fd < 0) {
        FATAL("create_tcp_server error:");
    }

    recv_and_check(sock_fd);

    sprintf(line, "HELO %s\r\n", sender_server);
    ret = send(sock_fd, line, strlen(line), 0);
    recv_and_check(sock_fd);

    sprintf(line, "MAIL FROM: <%s>\r\n", sender);
    ret = send(sock_fd, line, strlen(line), 0);
    recv_and_check(sock_fd);

    sprintf(line, "RCPT TO: <%s>\r\n", recipient);
    ret = send(sock_fd, line, strlen(line), 0);
    recv_and_check(sock_fd);

    sprintf(line, "DATA\r\n");
    ret = send(sock_fd, line, strlen(line), 0);
    recv_and_check(sock_fd);

    ret = send(sock_fd, mail_content, strlen(mail_content), 0);
    recv_and_check(sock_fd);

    sprintf(line, ".\r\n");
    ret = send(sock_fd, line, strlen(line), 0);
    recv_and_check(sock_fd);

    sprintf(line, "QUIT\r\n");
    ret = send(sock_fd, line, strlen(line), 0);
    recv_and_check(sock_fd);

    close(sock_fd);

    return 0;
}

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT            9000
#define DATA_FILE       "/var/tmp/aesdsocketdata"
#define RECV_CHUNK      1024
#define SEND_CHUNK      1024
#define BACKLOG         10

static volatile sig_atomic_t g_exit = 0;

static void signal_handler(int signo)
{
    (void)signo;
    g_exit = 1;
}

/* Append a chunk of data (already known-complete packet) to the file.
 * Returns 0 on success, -1 on failure. */
static int append_to_file(const char *data, size_t len)
{
    int fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        syslog(LOG_ERR, "open(%s) for append failed: %s",
               DATA_FILE, strerror(errno));
        return -1;
    }

    size_t written = 0;
    while (written < len) {
        ssize_t n = write(fd, data + written, len - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "write to %s failed: %s",
                   DATA_FILE, strerror(errno));
            close(fd);
            return -1;
        }
        written += (size_t)n;
    }

    close(fd);
    return 0;
}

/* Send the entire contents of DATA_FILE to the client socket.
 * Reads in chunks so it does not require the full file to fit in RAM.
 * Returns 0 on success, -1 on failure. */
static int send_full_file(int client_fd)
{
    int fd = open(DATA_FILE, O_RDONLY);
    if (fd < 0) {
        syslog(LOG_ERR, "open(%s) for read failed: %s",
               DATA_FILE, strerror(errno));
        return -1;
    }

    char buf[SEND_CHUNK];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t sent = 0;
        while (sent < n) {
            ssize_t s = send(client_fd, buf + sent, (size_t)(n - sent), 0);
            if (s < 0) {
                if (errno == EINTR) continue;
                syslog(LOG_ERR, "send() failed: %s", strerror(errno));
                close(fd);
                return -1;
            }
            sent += s;
        }
    }
    if (n < 0) {
        syslog(LOG_ERR, "read(%s) failed: %s", DATA_FILE, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/* Handle one accepted client connection.
 * Reads newline-delimited packets, appends each to the file,
 * and sends back the full file content after each complete packet.
 * Returns 0 on success, -1 on error. */
static int handle_client(int client_fd, const char *client_ip)
{
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);

    /* Buffer accumulating the current (incomplete) packet. */
    size_t   pkt_cap  = RECV_CHUNK;
    size_t   pkt_len  = 0;
    char    *pkt_buf  = malloc(pkt_cap);
    if (!pkt_buf) {
        syslog(LOG_ERR, "malloc failed for packet buffer");
        return -1;
    }

    char recv_buf[RECV_CHUNK];
    int  ret = 0;

    while (!g_exit) {
        ssize_t n = recv(client_fd, recv_buf, sizeof(recv_buf), 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "recv() failed: %s", strerror(errno));
            ret = -1;
            break;
        }
        if (n == 0) {
            /* Client closed connection. */
            break;
        }

        /* Process byte by byte, looking for newlines. */
        for (ssize_t i = 0; i < n; i++) {
            /* Grow packet buffer if needed (append data + '\0'). */
            if (pkt_len + 1 >= pkt_cap) {
                size_t new_cap = pkt_cap * 2;
                char *tmp = realloc(pkt_buf, new_cap);
                if (!tmp) {
                    syslog(LOG_ERR, "realloc failed, discarding packet");
                    pkt_len = 0;  /* discard over-length packet */
                    break;
                }
                pkt_buf = tmp;
                pkt_cap = new_cap;
            }

            pkt_buf[pkt_len++] = recv_buf[i];

            if (recv_buf[i] == '\n') {
                /* Complete packet: append to file. */
                if (append_to_file(pkt_buf, pkt_len) < 0) {
                    ret = -1;
                    goto done;
                }
                /* Then return full file contents to client. */
                if (send_full_file(client_fd) < 0) {
                    ret = -1;
                    goto done;
                }
                /* Reset packet buffer for next packet. */
                pkt_len = 0;
            }
        }
    }

done:
    free(pkt_buf);
    syslog(LOG_INFO, "Closed connection from %s", client_ip);
    return ret;
}

static void daemonize(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "fork() failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        /* Parent exits; the child continues in background. */
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        syslog(LOG_ERR, "setsid() failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Second fork so the daemon cannot reacquire a controlling terminal. */
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "second fork() failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "chdir(/) failed: %s", strerror(errno));
    }

    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO) close(devnull);
    }
}

int main(int argc, char *argv[])
{
    int daemon_mode = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            daemon_mode = 1;
        } else {
            fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Bind must succeed before we daemonize (per assignment spec).
     * So we bind here, then fork, then enter the accept loop. */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "socket() failed: %s", strerror(errno));
        closelog();
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        syslog(LOG_ERR, "bind() to port %d failed: %s",
               PORT, strerror(errno));
        close(server_fd);
        closelog();
        return EXIT_FAILURE;
    }

    if (daemon_mode) {
        daemonize();
    }

    if (listen(server_fd, BACKLOG) < 0) {
        syslog(LOG_ERR, "listen() failed: %s", strerror(errno));
        close(server_fd);
        closelog();
        return EXIT_FAILURE;
    }

    while (!g_exit) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "accept() failed: %s", strerror(errno));
            break;
        }

        char ip_str[INET_ADDRSTRLEN];
        if (!inet_ntop(AF_INET, &client_addr.sin_addr,
                       ip_str, sizeof(ip_str))) {
            strncpy(ip_str, "unknown", sizeof(ip_str) - 1);
            ip_str[sizeof(ip_str) - 1] = '\0';
        }

        handle_client(client_fd, ip_str);
        close(client_fd);
    }

    /* Graceful shutdown. */
    if (g_exit) {
        syslog(LOG_INFO, "Caught signal, exiting");
    }
    close(server_fd);
    unlink(DATA_FILE);

    closelog();
    return EXIT_SUCCESS;
}

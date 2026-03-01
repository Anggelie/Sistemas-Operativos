#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHM_NAME "/cc3064_lab2_shm"
#define SOCK_PATH "/tmp/cc3064_lab2_sock"
#define SHM_DATA 2048

typedef struct {
    volatile int idx;          
    char data[SHM_DATA];     

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static void send_fd(int sock, int fd) {
    struct msghdr msg = {0};
    char buf[1] = { 'F' };
    struct iovec io = { .iov_base = buf, .iov_len = sizeof(buf) };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    memset(cmsgbuf, 0, sizeof(cmsgbuf));
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(int));

    memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));
    msg.msg_controllen = cmsg->cmsg_len;

    if (sendmsg(sock, &msg, 0) < 0) die("sendmsg");
}

static int recv_fd(int sock) {
    struct msghdr msg = {0};
    char m_buffer[1];
    struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    memset(cmsgbuf, 0, sizeof(cmsgbuf));
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    if (recvmsg(sock, &msg, 0) < 0) die("recvmsg");

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg) return -1;

    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
}

static int make_server_socket(void) {
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) die("socket");

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCK_PATH); // limpia socket viejo
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) die("bind");
    if (listen(s, 1) < 0) die("listen");
    return s;
}

static int make_client_socket(void) {
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) die("socket");

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    for (int i = 0; i < 50; i++) {
        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == 0) return s;
        usleep(20000); // 20ms
    }
    die("connect");
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <n> <x>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s 5 a\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    char x = argv[2][0];
    if (n <= 0) {
        fprintf(stderr, "n debe ser > 0\n");
        return 1;
    }

    int created = 0;
    int shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (shm_fd >= 0) {
        created = 1;
        if (ftruncate(shm_fd, sizeof(Shared)) < 0) die("ftruncate");
        printf("%c: Created new shared mem obj\n", x);
    } else {
        if (errno == EEXIST) {
            shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
            if (shm_fd < 0) die("shm_open existing");
            printf("%c: Shared mem obj already created\n", x);
        } else {
            die("shm_open");
        }
    }

    if (created) {
        int srv = make_server_socket();
        int cli = accept(srv, NULL, NULL);
        if (cli < 0) die("accept");
        send_fd(cli, shm_fd);

        close(cli);
        close(srv);
        unlink(SOCK_PATH);
    } else {
        int s = make_client_socket();
        int other_fd = recv_fd(s);
        printf("%c: Received shm fd from other instance: %d\n", x, other_fd);
        close(other_fd); // NO se usa para mmap
        close(s);
    }

    Shared *sh = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sh == MAP_FAILED) die("mmap");

    if (created) {
        sh->idx = 0;
        memset(sh->data, 0, sizeof(sh->data));
    }

    int pfd[2];
    if (pipe(pfd) < 0) die("pipe");

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        close(pfd[1]); 
        char c;
        while (read(pfd[0], &c, 1) > 0) {
            // reservar una posición de manera atómica
            int pos = __sync_fetch_and_add(&sh->idx, 1);
            if (pos >= SHM_DATA) break;
            sh->data[pos] = c;
        }
        close(pfd[0]);
        munmap(sh, sizeof(Shared));
        close(shm_fd);
        _exit(0);
    } else {
        close(pfd[0]); 

        for (int i = 0; i < SHM_DATA; i++) {
            if (i % n == 0) {
                if (write(pfd[1], &x, 1) < 0) die("write pipe");
            }
        }

        close(pfd[1]); 
        wait(NULL);

        // Mostrar shm
        int limit = sh->idx;
        if (limit > SHM_DATA) limit = SHM_DATA;

        printf("%c: Shared memory has: ", x);
        for (int i = 0; i < limit; i++) putchar(sh->data[i]);
        putchar('\n');

        // cleanup
        munmap(sh, sizeof(Shared));
        close(shm_fd);

        if (created) {
            if (shm_unlink(SHM_NAME) < 0) perror("shm_unlink");
        }
    }

    return 0;
}
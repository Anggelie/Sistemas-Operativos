#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(void) {
    pid_t a = fork();
    if (a < 0) { perror("fork a"); return 1; }

    if (a == 0) {
        execl("./ipc", "ipc", "5", "a", (char*)NULL);
        perror("execl ipc a");
        _exit(1);
    }

    usleep(50000); // 50ms

    pid_t b = fork();
    if (b < 0) { perror("fork b"); return 1; }

    if (b == 0) {
        execl("./ipc", "ipc", "5", "b", (char*)NULL);
        perror("execl ipc b");
        _exit(1);
    }

    wait(NULL);
    wait(NULL);
    return 0;
}
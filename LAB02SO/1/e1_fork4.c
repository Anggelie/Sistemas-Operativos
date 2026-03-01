#include <stdio.h>
#include <unistd.h>

int main(void) {
    fork();
    fork();
    fork();
    fork();

    printf("PID=%d, PPID=%d\n", getpid(), getppid());
    return 0;
}
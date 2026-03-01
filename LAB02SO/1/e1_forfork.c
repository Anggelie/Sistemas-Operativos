#include <stdio.h>
#include <unistd.h>

int main(void) {
    for (int i = 0; i < 4; i++) {
        fork();
    }
    printf("PID=%d, PPID=%d\n", getpid(), getppid());
    return 0;
}
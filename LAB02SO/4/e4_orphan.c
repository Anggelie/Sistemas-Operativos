#include <stdio.h>
#include <unistd.h>

int main(void) {
    pid_t p = fork();

    if (p < 0) { perror("fork"); return 1; }

    if (p == 0) {
        // Hijo tarda en terminar
        for (int i = 1; i <= 4000000; i++) {
            printf("%d\n", i);
        }
        _exit(0);
    } else {
        while (1) { } // padre infinito
    }
}
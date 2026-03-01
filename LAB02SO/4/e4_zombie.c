#include <stdio.h>
#include <unistd.h>

int main(void) {
    pid_t p = fork();

    if (p < 0) { perror("fork"); return 1; }

    if (p == 0) {
        printf("Hijo: terminé.\n");
        _exit(0);
    } else {
        // Padre NO hace wait()
        while (1) { } // infinito
    }
}
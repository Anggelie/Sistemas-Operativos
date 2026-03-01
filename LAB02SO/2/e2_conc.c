#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t p1, p2, p3;
    clock_t start, end;

    start = clock();
    p1 = fork();

    if (p1 < 0) { perror("fork p1"); return 1; }

    if (p1 == 0) {
        // Hijo
        p2 = fork();
        if (p2 < 0) { perror("fork p2"); return 1; }

        if (p2 == 0) {
            // Nieto
            p3 = fork();
            if (p3 < 0) { perror("fork p3"); return 1; }

            if (p3 == 0) {
                // Bisnieto
                for (int i = 0; i < 1000000; i++) {}
                _exit(0);
            } else {
                // Nieto hace su for y espera al bisnieto
                for (int i = 0; i < 1000000; i++) {}
                wait(NULL);
                _exit(0);
            }
        } else {
            // Hijo hace su for y espera al nieto
            for (int i = 0; i < 1000000; i++) {}
            wait(NULL);
            _exit(0);
        }

    } else {
        // Padre espera al hijo
        wait(NULL);
        end = clock();
        double diff = (double)(end - start);
        printf("Ticks (clock diff): %f\n", diff);
    }

    return 0;
}
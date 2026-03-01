#include <stdio.h>
#include <time.h>

int main(void) {
    clock_t start, end;
    start = clock();

    for (int i = 0; i < 1000000; i++) {}
    for (int i = 0; i < 1000000; i++) {}
    for (int i = 0; i < 1000000; i++) {}

    end = clock();

    double diff = (double)(end - start);
    printf("Ticks (clock diff): %f\n", diff);
    return 0;
}
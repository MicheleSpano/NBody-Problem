#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {

    if(argc < 2) {
        fprintf(stderr, "Invalid argument, specify the number of bodies you want to create\n"); fflush(stdout);
        return -1;
    }

    FILE *file;
    int n = atoi(argv[1]);
    srand(time(NULL));

    clock_t begin = clock();

    if((file = fopen("inBodies.txt", "w+")) == NULL) {
        fprintf(stderr, "Can't create the file\n"); fflush(stdout);
        return -1;
    }

    fprintf(file, "%d\n", n);

    for(int i = 0; i < n; i++) {
        for(int j = 0; j < 6; j++) {
            fprintf(file, "%f ", rand() / (float)RAND_MAX);
        }
        if (i < n-1) fprintf(file, "\n");
    }

    fclose(file);

    clock_t end = clock();
    float time_spent = (float)(end - begin) / CLOCKS_PER_SEC;
    printf("Done in %f seconds\n", time_spent); fflush(stdout);

    return 0;
}
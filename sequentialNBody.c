#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#define SOFTENING 1e-9f

typedef struct {float x, y, z, vx, vy, vz;} Body;
typedef struct {float *buffer; int numBodies;} FileInfo;

FileInfo getInfoFromFile() {
    FILE *inputFile;
    int numBodies, buffIndex = 0;
    float *buff;
    char *singleNumber;

    if(((inputFile = fopen("inBodies.txt", "r")) == NULL)) {
        fprintf(stderr, "Can't read the input file\n"); fflush(stdout);
        exit(EXIT_FAILURE);
    }

    singleNumber = malloc(11*sizeof(char));

    fscanf(inputFile, "%s", singleNumber);
    numBodies = atoi(singleNumber);
    buff = (float*)malloc(numBodies*6*sizeof(float));

    while((fscanf(inputFile, "%s ", singleNumber)) != EOF) {
        *(buff+buffIndex) = atof(singleNumber);
        buffIndex++;
    }

    fclose(inputFile);
    free(singleNumber);
    FileInfo info = {buff, numBodies};
    return info;
}

void bodyForce(Body *p, float dt, int numBodies) {
    for (int i = 0; i < numBodies; i++) { 
        float Fx = 0.0f; float Fy = 0.0f; float Fz = 0.0f;

        for (int j = 0; j < numBodies; j++) {
            float dx = p[j].x - p[i].x;
            float dy = p[j].y - p[i].y;
            float dz = p[j].z - p[i].z;
            float distSqr = dx*dx + dy*dy + dz*dz + SOFTENING;
            float invDist = 1.0f / sqrtf(distSqr);
            float invDist3 = invDist * invDist * invDist;

            Fx += dx * invDist3; Fy += dy * invDist3; Fz += dz * invDist3;
        }

        p[i].vx += dt*Fx; p[i].vy += dt*Fy; p[i].vz += dt*Fz;
    }
}

void writeOutputBodies(Body *body, int numBodies) {
    FILE *outputFile;

    if((outputFile = fopen("outBodiesSequential.txt", "w")) == NULL) {
        fprintf(stderr, "Can't open the output file\n"); fflush(stdout);
        exit(EXIT_FAILURE);
    }

    fprintf(outputFile, "%d\n", numBodies);
    for(int i = 0; i < numBodies; i++) {
        fprintf(outputFile, "%lf %lf %lf %lf %lf %lf\n",
            body[i].x, body[i].y, body[i].z, body[i].vx, body[i].vy, body[i].vz);
    }

    fclose(outputFile);
}

int main(int argc, char *argv[]) {

    if(argc < 2) {
        fprintf(stderr, "Specify the number of iterations in argv[1]\n"); fflush(stdout);
        exit(EXIT_FAILURE);
    }

    FileInfo info;
    int numBodies, numIterations;
    Body *body;
    const float dt = 0.01f;

    clock_t begin = clock();
    
    info = (FileInfo) getInfoFromFile();
    numBodies = info.numBodies;
    body = (Body*) info.buffer;
    numIterations = atoi(argv[1]);

    for(int i = 0; i < numIterations; i++) {
        bodyForce(body, dt, numBodies);

        for (int i = 0 ; i < numBodies; i++) {
            body[i].x += body[i].vx*dt;
            body[i].y += body[i].vy*dt;
            body[i].z += body[i].vz*dt;
        }

        clock_t partial = clock();
        printf("Elapsed time after iteration %d: %f\n", i+1, (float) (partial - begin) / CLOCKS_PER_SEC); fflush(stdout);
    }

    writeOutputBodies(body, numBodies);
    free(info.buffer);
    return 0;
}
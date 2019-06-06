#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#define SOFTENING 1e-9f

typedef struct {float x, y, z, vx, vy, vz;} Body;
typedef struct {float *buffer; int numBodies;} FileInfo;

FileInfo getInfoFromFile() {
    FILE *bodiesFile;
    int numBodies, buffIndex = 0;
    float *buff;
    char *singleNumber;

    if(((bodiesFile = fopen("inBodies.txt", "r")) == NULL)) {
        fprintf(stderr, "Can't read the input file\n"); fflush(stdout);
        exit(EXIT_FAILURE);
    }

    singleNumber = malloc(11*sizeof(char));

    fscanf(bodiesFile, "%s", singleNumber);
    numBodies = atoi(singleNumber);
    buff = (float*)malloc(numBodies*6*sizeof(float));

    while((fscanf(bodiesFile, "%s ", singleNumber)) != EOF) {
        *(buff+buffIndex) = atof(singleNumber);
        buffIndex++;
    }

    fclose(bodiesFile);
    free(singleNumber);
    FileInfo info = {buff, numBodies};
    return info;
}

void bodyForce(Body *allBodies, float dt, int numBodies, Body *subBodies, int subNumBodies) {
    for (int i = 0; i < subNumBodies; i++) { 
        float Fx = 0.0f; float Fy = 0.0f; float Fz = 0.0f;

        for (int j = 0; j < numBodies; j++) {
            float dx = allBodies[j].x - subBodies[i].x;
            float dy = allBodies[j].y - subBodies[i].y;
            float dz = allBodies[j].z - subBodies[i].z;
            float distSqr = dx*dx + dy*dy + dz*dz + SOFTENING;
            float invDist = 1.0f / sqrtf(distSqr);
            float invDist3 = invDist * invDist * invDist;

            Fx += dx * invDist3; Fy += dy * invDist3; Fz += dz * invDist3;
        }

        subBodies[i].vx += dt*Fx; subBodies[i].vy += dt*Fy; subBodies[i].vz += dt*Fz;
    }
}

void writeOutputBodies(Body *body, int numBodies) {
    FILE *outputFile;

    if((outputFile = fopen("outBodiesParallel.txt", "w")) == NULL) {
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
        return -1;
    }

    int worldSize, rank;
    const int count = 6;
    int array_of_blocklengths[6];
    MPI_Aint array_of_displacement[6];
    MPI_Datatype array_of_types[6], mpi_body_type;
    FileInfo inFIle;
    Body *body, *subBodies;
    int numBodies, subNumBodies, module, *sendcounts,
        *displs, offset = 0, numIterations;
    const float dt = 0.01f;
    double begin, partial;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    begin = MPI_Wtime();

    //Creation of the new datatype
    for(int i = 0; i < 6; i++) {
        array_of_blocklengths[i] = 1;
        array_of_types[i] = MPI_FLOAT;
    }

    array_of_displacement[0] = offsetof(Body, x);
    array_of_displacement[1] = offsetof(Body, y);
    array_of_displacement[2] = offsetof(Body, z);
    array_of_displacement[3] = offsetof(Body, vx);
    array_of_displacement[4] = offsetof(Body, vy);
    array_of_displacement[5] = offsetof(Body, vz);

    MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacement, array_of_types, &mpi_body_type);
    MPI_Type_commit(&mpi_body_type);

    inFIle = getInfoFromFile();
    body = (Body*) inFIle.buffer;
    numBodies = inFIle.numBodies;

    subNumBodies = numBodies / worldSize;
    module = numBodies % worldSize;

    sendcounts = malloc(worldSize*sizeof(int));
    displs = malloc(worldSize*sizeof(int));
    subBodies = malloc((subNumBodies+1)*sizeof(Body));

    for (int i = 0; i < worldSize; i++) {
        sendcounts[i] = subNumBodies;
        if (module > 0) {
            sendcounts[i]++;
            module--;
        }
        displs[i] = offset;
        offset += sendcounts[i];
    }

    numIterations = atoi(argv[1]);
    for(int i = 0; i < numIterations; i++) {
        MPI_Scatterv(body, sendcounts, displs, mpi_body_type, subBodies, subNumBodies+1, mpi_body_type, 0, MPI_COMM_WORLD);
        bodyForce(body, dt, numBodies, subBodies, sendcounts[rank]);
        MPI_Allgatherv(subBodies, sendcounts[rank], mpi_body_type, body, sendcounts, displs, mpi_body_type, MPI_COMM_WORLD);

        for (int i = 0 ; i < numBodies; i++) {
            body[i].x += body[i].vx*dt;
            body[i].y += body[i].vy*dt;
            body[i].z += body[i].vz*dt;
        }

        partial = MPI_Wtime();
        if(rank == 0)
            printf("Elapsed time after iteration %d: %lf\n", i+1, partial-begin); fflush(stdout);
    }

    if(rank == 0)
        writeOutputBodies(body, numBodies);

    free(body);
    free(subBodies);
    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return 0;
}
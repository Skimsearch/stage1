#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Cross-platform directory creation support
#ifdef _WIN32
#include <direct.h>
#define create_dir(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define create_dir(path) mkdir(path, 0777)
#endif

// Setup directory hierarchy for the Batch strategy
void setup_directories() {
    create_dir("datalake_c");
    create_dir("datalake_c/batch");
    create_dir("datalake_c/batch/0-999");
}

int main() {
    const char *dummyBody = "Simulated body content of a Gutenberg book to measure disk I/O performance in C.";
    
    // Start high-precision timer
    clock_t startTime = clock();

    // Create datalake folder structure
    setup_directories();

    char filepath[256];
    for (int bookId = 0; bookId < 1000; bookId++) {
        // Build file path (e.g., datalake_c/batch/0-999/42.body.txt)
        sprintf(filepath, "datalake_c/batch/0-999/%d.body.txt", bookId);
        
        // Open file in write mode ("w")
        FILE *file = fopen(filepath, "w");
        if (file != NULL) {
            fputs(dummyBody, file);
            fclose(file); // Close immediately to release file descriptor
        } else {
            fprintf(stderr, "Error creating file: %s\n", filepath);
        }
    }

    clock_t endTime = clock();
    
    // Calculate elapsed time in milliseconds
    double durationMs = 1000.0 * (double)(endTime - startTime) / CLOCKS_PER_SEC;

    printf("Batch Strategy (C) - Total time: %.2f ms\n", durationMs);

    return 0;
}
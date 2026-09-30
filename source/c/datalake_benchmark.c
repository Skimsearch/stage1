#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>


int books[] = {
    1342,
    11,
    84,
    98,
    1661,
    2701
};

int number_of_books = 6;


/* -----------------------------
   LOOKUP
----------------------------- */

int find_book_book_strategy(int book_id) {

    char path[256];

    sprintf(
        path,
        "datalake_c/book/%d/%d.body.txt",
        book_id,
        book_id
    );

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return 0;
    }

    fclose(file);

    return 1;
}


int find_book_batch_strategy(int book_id) {

    int batch_start =
        (book_id / 1000) * 1000;

    int batch_end =
        batch_start + 999;

    char path[256];

    sprintf(
        path,
        "datalake_c/batch/%d-%d/%d.body.txt",
        batch_start,
        batch_end,
        book_id
    );

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return 0;
    }

    fclose(file);

    return 1;
}


int find_book_time_strategy(int book_id) {

    DIR *date_dir =
        opendir("datalake_c/time");

    if (date_dir == NULL) {
        return 0;
    }

    struct dirent *date_entry;

    while (
        (date_entry = readdir(date_dir))
        != NULL
    ) {

        if (date_entry->d_name[0] == '.') {
            continue;
        }

        char date_path[256];

        sprintf(
            date_path,
            "datalake_c/time/%s",
            date_entry->d_name
        );

        DIR *hour_dir =
            opendir(date_path);

        if (hour_dir == NULL) {
            continue;
        }

        struct dirent *hour_entry;

        while (
            (hour_entry = readdir(hour_dir))
            != NULL
        ) {

            if (hour_entry->d_name[0] == '.') {
                continue;
            }

            char book_path[512];

            sprintf(
                book_path,
                "%s/%s/%d.body.txt",
                date_path,
                hour_entry->d_name,
                book_id
            );

            FILE *file =
                fopen(book_path, "r");

            if (file != NULL) {

                fclose(file);
                closedir(hour_dir);
                closedir(date_dir);

                return 1;
            }
        }

        closedir(hour_dir);
    }

    closedir(date_dir);

    return 0;
}


/* -----------------------------
   LOOKUP BENCHMARK
----------------------------- */

void benchmark_lookup() {

    int repetitions = 1000;

    int test_book = 1342;

    printf("\nLOOKUP BENCHMARK\n");


    clock_t start = clock();

    for (int i = 0; i < repetitions; i++) {
        find_book_book_strategy(test_book);
    }

    double book_time =
        (double)(clock() - start)
        / CLOCKS_PER_SEC;


    start = clock();

    for (int i = 0; i < repetitions; i++) {
        find_book_batch_strategy(test_book);
    }

    double batch_time =
        (double)(clock() - start)
        / CLOCKS_PER_SEC;


    start = clock();

    for (int i = 0; i < repetitions; i++) {
        find_book_time_strategy(test_book);
    }

    double time_time =
        (double)(clock() - start)
        / CLOCKS_PER_SEC;


    printf(
        "\nBook strategy: %.6f seconds\n",
        book_time
    );

    printf(
        "Batch strategy: %.6f seconds\n",
        batch_time
    );

    printf(
        "Time strategy: %.6f seconds\n",
        time_time
    );
}


/* -----------------------------
   STORAGE
----------------------------- */

long folder_size(const char *path) {

    DIR *dir = opendir(path);

    if (dir == NULL) {
        return 0;
    }

    long total_size = 0;

    struct dirent *entry;

    while (
        (entry = readdir(dir))
        != NULL
    ) {

        if (
            strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0
        ) {
            continue;
        }

        char full_path[512];

        sprintf(
            full_path,
            "%s/%s",
            path,
            entry->d_name
        );

        struct stat info;

        stat(full_path, &info);

        if (S_ISDIR(info.st_mode)) {

            total_size +=
                folder_size(full_path);
        }

        else {

            total_size +=
                info.st_size;
        }
    }

    closedir(dir);

    return total_size;
}


void benchmark_storage() {

    printf("\nSTORAGE OVERHEAD BENCHMARK\n");

    long book_size =
        folder_size("datalake_c/book");

    long batch_size =
        folder_size("datalake_c/batch");

    long time_size =
        folder_size("datalake_c/time");


    printf(
        "\nBook: %.2f KB\n",
        book_size / 1024.0
    );

    printf(
        "Batch: %.2f KB\n",
        batch_size / 1024.0
    );

    printf(
        "Time: %.2f KB\n",
        time_size / 1024.0
    );
}

int is_indexed(int book_id) {

    int indexed_books[] = {
        11,
        84
    };

    int indexed_count = 2;

    for (int i = 0; i < indexed_count; i++) {

        if (indexed_books[i] == book_id) {
            return 1;
        }
    }

    return 0;
}


void benchmark_incremental_processing() {

    printf("\nINCREMENTAL PROCESSING BENCHMARK\n");

    clock_t start = clock();

    printf("\nBooks ready to index: ");

    for (int i = 0; i < number_of_books; i++) {

        int book_id = books[i];

        if (
            find_book_book_strategy(book_id)
            && !is_indexed(book_id)
        ) {

            printf("%d ", book_id);
        }
    }

    double elapsed =
        (double)(clock() - start)
        / CLOCKS_PER_SEC;

    printf(
        "\nDetection time: %.6f seconds\n",
        elapsed
    );
}

int check_book_complete_book_strategy(int book_id) {

    char header_path[256];
    char body_path[256];

    sprintf(
        header_path,
        "datalake_c/book/%d/%d.header.txt",
        book_id,
        book_id
    );

    sprintf(
        body_path,
        "datalake_c/book/%d/%d.body.txt",
        book_id,
        book_id
    );

    FILE *header = fopen(header_path, "r");
    FILE *body = fopen(body_path, "r");

    int header_exists = header != NULL;
    int body_exists = body != NULL;

    if (header != NULL) {
        fclose(header);
    }

    if (body != NULL) {
        fclose(body);
    }

    if (header_exists && body_exists) {
        return 2;   // complete
    }

    if (header_exists || body_exists) {
        return 1;   // incomplete
    }

    return 0;       // missing
}


void benchmark_recovery_behavior() {

    printf("\nRECOVERY BEHAVIOR BENCHMARK\n");

    int book_id = 1342;

    char body_path[256];
    char backup_path[256];

    sprintf(
        body_path,
        "datalake_c/book/%d/%d.body.txt",
        book_id,
        book_id
    );

    sprintf(
        backup_path,
        "datalake_c/book/%d/%d.body.txt.bak",
        book_id,
        book_id
    );

    rename(
        body_path,
        backup_path
    );

    clock_t start = clock();

    int state =
        check_book_complete_book_strategy(
            book_id
        );

    double elapsed =
        (double)(clock() - start)
        / CLOCKS_PER_SEC;


    if (state == 2) {

        printf(
            "Book %d state: complete\n",
            book_id
        );
    }

    else if (state == 1) {

        printf(
            "Book %d state: incomplete\n",
            book_id
        );
    }

    else {

        printf(
            "Book %d state: missing\n",
            book_id
        );
    }

    printf(
        "Recovery detection time: %.6f seconds\n",
        elapsed
    );


    rename(
        backup_path,
        body_path
    );

    printf(
        "Book %d restored\n",
        book_id
    );
}

void benchmark_download_write_throughput() {

    printf("\nDOWNLOAD / WRITE THROUGHPUT BENCHMARK\n");

    struct timespec start;
    struct timespec end;

    clock_gettime(
        CLOCK_MONOTONIC,
        &start
    );

    int result =
        system("./source/c/crawler > /dev/null");

    clock_gettime(
        CLOCK_MONOTONIC,
        &end
    );

    if (result != 0) {

        printf("Error running crawler\n");
        return;
    }

    double elapsed =
        (end.tv_sec - start.tv_sec)
        +
        (end.tv_nsec - start.tv_nsec)
        / 1000000000.0;

    printf(
        "Total crawler time: %.6f seconds\n",
        elapsed
    );

    printf(
        "Average per book: %.6f seconds\n",
        elapsed / number_of_books
    );
}

int main() {

    benchmark_lookup();

    benchmark_storage();

    benchmark_incremental_processing();

    benchmark_recovery_behavior();

    benchmark_download_write_throughput();

    return 0;
}
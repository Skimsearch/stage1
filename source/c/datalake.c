#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>


void create_directory(const char *path) {
    mkdir(path, 0777);
}


int write_text_file(
    const char *path,
    const char *content
) {

    FILE *file = fopen(path, "w");

    if (file == NULL) {
        return 0;
    }

    fprintf(file, "%s", content);

    fclose(file);

    return 1;
}


int save_book(
    int book_id,
    const char *header,
    const char *body,
    const char *strategy
) {

    char folder_path[256];
    char header_path[256];
    char body_path[256];

    create_directory("datalake_c");


    if (strcmp(strategy, "book") == 0) {

        create_directory("datalake_c/book");

        sprintf(
            folder_path,
            "datalake_c/book/%d",
            book_id
        );

        create_directory(folder_path);
    }


    else if (strcmp(strategy, "batch") == 0) {

        int batch_start =
            (book_id / 1000) * 1000;

        int batch_end =
            batch_start + 999;

        create_directory("datalake_c/batch");

        sprintf(
            folder_path,
            "datalake_c/batch/%d-%d",
            batch_start,
            batch_end
        );

        create_directory(folder_path);
    }


    else if (strcmp(strategy, "time") == 0) {

        time_t now = time(NULL);

        struct tm *current_time =
            localtime(&now);

        char date_folder[64];
        char hour_folder[64];

        strftime(
            date_folder,
            sizeof(date_folder),
            "%Y%m%d",
            current_time
        );

        strftime(
            hour_folder,
            sizeof(hour_folder),
            "%H",
            current_time
        );

        create_directory("datalake_c/time");

        char date_path[256];

        sprintf(
            date_path,
            "datalake_c/time/%s",
            date_folder
        );

        create_directory(date_path);

        sprintf(
            folder_path,
            "%s/%s",
            date_path,
            hour_folder
        );

        create_directory(folder_path);
    }


    else {

        printf(
            "Unknown strategy: %s\n",
            strategy
        );

        return 0;
    }


    sprintf(
        header_path,
        "%s/%d.header.txt",
        folder_path,
        book_id
    );

    sprintf(
        body_path,
        "%s/%d.body.txt",
        folder_path,
        book_id
    );


    if (!write_text_file(
        header_path,
        header
    )) {

        printf(
            "Error saving header for book %d\n",
            book_id
        );

        return 0;
    }


    if (!write_text_file(
        body_path,
        body
    )) {

        printf(
            "Error saving body for book %d\n",
            book_id
        );

        return 0;
    }


    printf(
        "Book %d saved using %s strategy\n",
        book_id,
        strategy
    );

    return 1;
}
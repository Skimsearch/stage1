#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>


void create_directory(const char *path) {

    mkdir(path, 0777);
}


int save_book(
    int book_id,
    const char *header,
    const char *body
) {

    char folder_path[256];
    char header_path[256];
    char body_path[256];

    create_directory("datalake_c");
    create_directory("datalake_c/book");

    sprintf(
        folder_path,
        "datalake_c/book/%d",
        book_id
    );

    create_directory(folder_path);

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


    FILE *header_file = fopen(
        header_path,
        "w"
    );

    if (header_file == NULL) {

        printf(
            "Error saving header for book %d\n",
            book_id
        );

        return 0;
    }

    fprintf(
        header_file,
        "%s",
        header
    );

    fclose(header_file);


    FILE *body_file = fopen(
        body_path,
        "w"
    );

    if (body_file == NULL) {

        printf(
            "Error saving body for book %d\n",
            book_id
        );

        return 0;
    }

    fprintf(
        body_file,
        "%s",
        body
    );

    fclose(body_file);


    printf(
        "Book %d saved in C datalake\n",
        book_id
    );

    return 1;
}
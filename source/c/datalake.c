#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

void create_directory(const char *path) {
    mkdir(path, 0777);
}


int copy_file(const char *source_path, const char *destination_path) {

    FILE *source = fopen(source_path, "rb");

    if (source == NULL) {
        printf("Error opening source file: %s\n", source_path);
        return 0;
    }

    FILE *destination = fopen(destination_path, "wb");

    if (destination == NULL) {
        printf("Error opening destination file: %s\n", destination_path);
        fclose(source);
        return 0;
    }

    char buffer[4096];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {

        fwrite(buffer, 1, bytes_read, destination);
    }

    fclose(source);
    fclose(destination);

    return 1;
}


void save_book_by_id(int book_id) {

    char source_path[256];
    char folder_path[256];
    char destination_path[256];

    sprintf(
        source_path,
        "datalake/book/%d/%d.body.txt",
        book_id,
        book_id
    );

    create_directory("datalake_c");
    create_directory("datalake_c/book");

    sprintf(
        folder_path,
        "datalake_c/book/%d",
        book_id
    );

    create_directory(folder_path);

    sprintf(
        destination_path,
        "%s/%d.body.txt",
        folder_path,
        book_id
    );

    if (copy_file(source_path, destination_path)) {

        printf(
            "Book %d copied successfully\n",
            book_id
        );
    }
}


int main() {

    int books[] = {
        1342,
        11,
        84,
        98,
        1661,
        2701
    };

    int number_of_books =
        sizeof(books) / sizeof(books[0]);

    for (int i = 0; i < number_of_books; i++) {

        save_book_by_id(books[i]);
    }

    return 0;
}
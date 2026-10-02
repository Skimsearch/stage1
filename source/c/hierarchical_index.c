#include <stdio.h>
#include <ctype.h>

#include "indexer.h"


void search_hierarchical(
    const char *term
) {

    char letter =
        (char)toupper(
            (unsigned char)term[0]
        );

    char path[512];

    sprintf(
        path,
        "datamart_c/inverted_index_hierarchical/%c/%s.txt",
        letter,
        term
    );

    FILE *file =
        fopen(
            path,
            "r"
        );

    printf(
        "%s: [",
        term
    );

    if (file == NULL) {

        printf("]\n");
        return;
    }

    int book_id;
    int first = 1;

    while (
        fscanf(
            file,
            "%d",
            &book_id
        ) == 1
    ) {

        if (!first) {
            printf(", ");
        }

        printf(
            "%d",
            book_id
        );

        first = 0;
    }

    fclose(file);

    printf("]\n");
}


int main(void) {

    int books[] = {
        1342,
        11,
        84,
        98,
        1661
    };

    int number_of_books =
        sizeof(books)
        /
        sizeof(books[0]);


    printf(
        "BUILDING C HIERARCHICAL INDEX\n\n"
    );


    for (
        int i = 0;
        i < number_of_books;
        i++
    ) {

        index_book(
            books[i]
        );
    }


    save_hierarchical_index();


    printf(
        "\nSEARCH EXAMPLES\n"
    );

    search_hierarchical(
        "pride"
    );

    search_hierarchical(
        "monster"
    );

    search_hierarchical(
        "nonexistentword"
    );


    reset_index();

    return 0;
}
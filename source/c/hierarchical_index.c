#include <stdio.h>
#include <ctype.h>

#include "indexer.h"


int query_hierarchical_count(const char *term) {

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

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return 0;
    }

    int book_id;
    int count = 0;

    while (
        fscanf(
            file,
            "%d",
            &book_id
        ) == 1
    ) {
        count++;
    }

    fclose(file);

    return count;
}

#include <stdio.h>
#include <ctype.h>
#include <direct.h>
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

typedef struct {
    int book_id;
} HierarchicalUpdateContext;


static void update_hierarchical_term(
    const char *term,
    const int *book_ids,
    int book_count,
    void *context
) {

    HierarchicalUpdateContext *ctx =
        (HierarchicalUpdateContext *)context;

    int contains_book = 0;

    for (int i = 0; i < book_count; i++) {

        if (book_ids[i] == ctx->book_id) {

            contains_book = 1;
            break;
        }
    }

    if (!contains_book) {
        return;
    }


    char letter =
        (char)toupper(
            (unsigned char)term[0]
        );


    char folder[256];

    sprintf(
        folder,
        "datamart_c/inverted_index_hierarchical/%c",
        letter
    );

    _mkdir(
        "datamart_c/inverted_index_hierarchical"
    );

    _mkdir(folder);


    char path[512];

    sprintf(
        path,
        "%s/%s.txt",
        folder,
        term
    );


    FILE *file =
        fopen(path, "a");

    if (file != NULL) {

        fprintf(
            file,
            "%d\n",
            ctx->book_id
        );

        fclose(file);
    }
}


void update_hierarchical_index_for_book(
    int book_id
) {

    HierarchicalUpdateContext context;

    context.book_id =
        book_id;

    for_each_term(
        update_hierarchical_term,
        &context
    );
}

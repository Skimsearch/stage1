#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mongoc/mongoc.h>
#include <bson/bson.h>

#define TABLE_SIZE 20011
#define MAX_BOOKS_PER_TERM 20
#define MAX_TOKEN_SIZE 256


typedef struct TermNode {

    char *term;

    int book_ids[MAX_BOOKS_PER_TERM];
    int book_count;

    struct TermNode *next;

} TermNode;


TermNode *hash_table[TABLE_SIZE] = {NULL};


unsigned long hash_term(const char *term) {

    unsigned long hash = 5381;

    int c;

    while ((c = *term++)) {

        hash = ((hash << 5) + hash) + c;
    }

    return hash % TABLE_SIZE;
}


TermNode *find_term(const char *term) {

    unsigned long index =
        hash_term(term);

    TermNode *current =
        hash_table[index];

    while (current != NULL) {

        if (
            strcmp(
                current->term,
                term
            ) == 0
        ) {

            return current;
        }

        current =
            current->next;
    }

    return NULL;
}


void add_term(
    const char *term,
    int book_id
) {

    unsigned long index =
        hash_term(term);

    TermNode *node =
        find_term(term);


    if (node == NULL) {

        node =
            malloc(
                sizeof(TermNode)
            );

        node->term =
            strdup(term);

        node->book_count = 0;

        node->next =
            hash_table[index];

        hash_table[index] =
            node;
    }


    for (
        int i = 0;
        i < node->book_count;
        i++
    ) {

        if (
            node->book_ids[i]
            == book_id
        ) {

            return;
        }
    }


    if (
        node->book_count
        < MAX_BOOKS_PER_TERM
    ) {

        node->book_ids[
            node->book_count
        ] = book_id;

        node->book_count++;
    }
}


int is_letter(char c) {

    return (
        (c >= 'a' && c <= 'z')
        ||
        (c >= 'A' && c <= 'Z')
    );
}


char to_lowercase(char c) {

    if (
        c >= 'A'
        &&
        c <= 'Z'
    ) {

        return c + ('a' - 'A');
    }

    return c;
}


void index_book(
    int book_id
) {

    char path[256];

    sprintf(
        path,
        "datalake_c/book/%d/%d.body.txt",
        book_id,
        book_id
    );


    FILE *file =
        fopen(path, "r");


    if (file == NULL) {

        printf(
            "Body for book %d not found\n",
            book_id
        );

        return;
    }


    char token[MAX_TOKEN_SIZE];

    int token_length = 0;

    int c;


    while (
        (c = fgetc(file))
        != EOF
    ) {

        if (
            is_letter((char)c)
        ) {

            if (
                token_length
                < MAX_TOKEN_SIZE - 1
            ) {

                token[token_length] =
                    to_lowercase(
                        (char)c
                    );

                token_length++;
            }
        }

        else {

            if (
                token_length > 0
            ) {

                token[token_length] =
                    '\0';

                add_term(
                    token,
                    book_id
                );

                token_length = 0;
            }
        }
    }


    if (
        token_length > 0
    ) {

        token[token_length] =
            '\0';

        add_term(
            token,
            book_id
        );
    }


    fclose(file);

    printf(
        "Book %d indexed\n",
        book_id
    );
}


void save_mongodb_index() {

    mongoc_client_t *client;
    mongoc_collection_t *collection;

    bson_error_t error;




    client =
        mongoc_client_new(
            "mongodb://localhost:27017"
        );


    collection =
        mongoc_client_get_collection(
            client,
            "stage1",
            "inverted_index"
        );


    bson_t empty =
        BSON_INITIALIZER;


    mongoc_collection_delete_many(
        collection,
        &empty,
        NULL,
        NULL,
        &error
    );


    for (
        int i = 0;
        i < TABLE_SIZE;
        i++
    ) {

        TermNode *current =
            hash_table[i];


        while (
            current != NULL
        ) {

            bson_t document;
            bson_t array;


            bson_init(
                &document
            );


            BSON_APPEND_UTF8(
                &document,
                "term",
                current->term
            );


            BSON_APPEND_ARRAY_BEGIN(
                &document,
                "book_ids",
                &array
            );


            for (
                int j = 0;
                j < current->book_count;
                j++
            ) {

                char key[16];

                snprintf(
                    key,
                    sizeof(key),
                    "%d",
                    j
                );


                BSON_APPEND_INT32(
                    &array,
                    key,
                    current->book_ids[j]
                );
            }


            bson_append_array_end(
                &document,
                &array
            );


            if (
                !mongoc_collection_insert_one(
                    collection,
                    &document,
                    NULL,
                    NULL,
                    &error
                )
            ) {

                printf(
                    "Mongo insert error: %s\n",
                    error.message
                );
            }


            bson_destroy(
                &document
            );


            current =
                current->next;
        }
    }


    mongoc_collection_destroy(
        collection
    );

    mongoc_client_destroy(
        client
    );



    printf(
        "MongoDB index saved\n"
    );
}


void search_mongodb(
    const char *term
) {

    mongoc_client_t *client;
    mongoc_collection_t *collection;
    mongoc_cursor_t *cursor;

    bson_t query;

    const bson_t *document;





    client =
        mongoc_client_new(
            "mongodb://localhost:27017"
        );


    collection =
        mongoc_client_get_collection(
            client,
            "stage1",
            "inverted_index"
        );


    bson_init(
        &query
    );


    BSON_APPEND_UTF8(
        &query,
        "term",
        term
    );


    cursor =
        mongoc_collection_find_with_opts(
            collection,
            &query,
            NULL,
            NULL
        );


    printf(
        "%s: ",
        term
    );


    if (
        mongoc_cursor_next(
            cursor,
            &document
        )
    ) {

        char *json =
            bson_as_canonical_extended_json(
                document,
                NULL
            );


        printf(
            "%s\n",
            json
        );


        bson_free(
            json
        );
    }

    else {

        printf(
            "not found\n"
        );
    }


    bson_destroy(
        &query
    );

    mongoc_cursor_destroy(
        cursor
    );

    mongoc_collection_destroy(
        collection
    );

    mongoc_client_destroy(
        client
    );

}


int main(void) {

    mongoc_init();

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
        "BUILDING C MONGODB INDEX\n\n"
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

    save_mongodb_index();

    printf(
        "\nSEARCH EXAMPLES\n"
    );

    search_mongodb(
        "pride"
    );

    search_mongodb(
        "monster"
    );

    search_mongodb(
        "nonexistentword"
    );

    mongoc_cleanup();

    return 0;
}
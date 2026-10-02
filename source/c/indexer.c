#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "indexer.h"

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

    unsigned long index = hash_term(term);

    TermNode *current = hash_table[index];

    while (current != NULL) {

        if (strcmp(current->term, term) == 0) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

int query_term_count(const char *term) {

    TermNode *node = find_term(term);

    if (node == NULL) {
        return 0;
    }

    return node->book_count;
}

void add_term(const char *term, int book_id) {

    unsigned long index = hash_term(term);

    TermNode *node = find_term(term);

    if (node == NULL) {

        node = malloc(sizeof(TermNode));

        node->term = strdup(term);

        node->book_count = 0;

        node->next = hash_table[index];

        hash_table[index] = node;
    }


    for (int i = 0; i < node->book_count; i++) {

        if (node->book_ids[i] == book_id) {
            return;
        }
    }


    if (node->book_count < MAX_BOOKS_PER_TERM) {

        node->book_ids[node->book_count] = book_id;

        node->book_count++;
    }
}

void reset_index(void) {

    for (int i = 0; i < TABLE_SIZE; i++) {

        TermNode *current = hash_table[i];

        while (current != NULL) {

            TermNode *next = current->next;

            free(current->term);
            free(current);

            current = next;
        }

        hash_table[i] = NULL;
    }
}

int get_unique_term_count(void) {

    int count = 0;

    for (int i = 0; i < TABLE_SIZE; i++) {

        TermNode *current = hash_table[i];

        while (current != NULL) {

            count++;

            current = current->next;
        }
    }

    return count;
}

int is_letter(char c) {

    return (
        (c >= 'a' && c <= 'z')
        ||
        (c >= 'A' && c <= 'Z')
    );
}


char to_lowercase(char c) {

    if (c >= 'A' && c <= 'Z') {

        return c + ('a' - 'A');
    }

    return c;
}


void index_book(int book_id) {

    char path[256];

    sprintf(
        path,
        "datalake/book/%d/%d.body.txt",
        book_id,
        book_id
    );

    FILE *file = fopen(path, "r");

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


    while ((c = fgetc(file)) != EOF) {

        if (is_letter((char)c)) {

            if (token_length < MAX_TOKEN_SIZE - 1) {

                token[token_length] =
                    to_lowercase((char)c);

                token_length++;
            }
        }

        else {

            if (token_length > 0) {

                token[token_length] = '\0';

                add_term(
                    token,
                    book_id
                );

                token_length = 0;
            }
        }
    }


    if (token_length > 0) {

        token[token_length] = '\0';

        add_term(
            token,
            book_id
        );
    }


    fclose(file);

}


void save_index() {

    _mkdir("datamart_c");
    FILE *file = fopen(
        "datamart_c/inverted_index.json",
        "w"
    );

    if (file == NULL) {

        printf("Error saving index\n");

        return;
    }


    fprintf(file, "{\n");

    int first_term = 1;


    for (int i = 0; i < TABLE_SIZE; i++) {

        TermNode *current =
            hash_table[i];

        while (current != NULL) {

            if (!first_term) {

                fprintf(file, ",\n");
            }

            first_term = 0;


            fprintf(
                file,
                "  \"%s\": [",
                current->term
            );


            for (
                int j = 0;
                j < current->book_count;
                j++
            ) {

                if (j > 0) {
                    fprintf(file, ", ");
                }

                fprintf(
                    file,
                    "%d",
                    current->book_ids[j]
                );
            }


            fprintf(file, "]");

            current = current->next;
        }
    }


    fprintf(file, "\n}\n");

    fclose(file);

}


void search_term(const char *term) {

    TermNode *node =
        find_term(term);

    printf(
        "%s: [",
        term
    );


    if (node != NULL) {

        for (
            int i = 0;
            i < node->book_count;
            i++
        ) {

            if (i > 0) {
                printf(", ");
            }

            printf(
                "%d",
                node->book_ids[i]
            );
        }
    }


    printf("]\n");
}



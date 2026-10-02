#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>
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

void for_each_term(
    term_callback callback,
    void *context
) {

    for (int i = 0; i < TABLE_SIZE; i++) {

        TermNode *current =
            hash_table[i];

        while (current != NULL) {

            callback(
                current->term,
                current->book_ids,
                current->book_count,
                context
            );

            current =
                current->next;
        }
    }
}

int is_letter(char c) {

    return (
        (c >= 'a' && c <= 'z')
        ||
        (c >= 'A' && c <= 'Z')
    );
}


int is_word_char(char c) {

    return (
        (c >= 'a' && c <= 'z')
        ||
        (c >= 'A' && c <= 'Z')
        ||
        (c >= '0' && c <= '9')
        ||
        c == '_'
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
    int valid_token = 1;

    int c;


    while ((c = fgetc(file)) != EOF) {

        if (is_word_char((char)c)) {

            if (is_letter((char)c)) {

                if (token_length < MAX_TOKEN_SIZE - 1) {

                    token[token_length] =
                        to_lowercase((char)c);

                    token_length++;
                }
            }

            else {

                /*
                 * A digit or underscore inside a word means
                 * it does not match \b[a-zA-Z]+\b.
                 */
                valid_token = 0;
            }
        }

        else {

            if (token_length > 0 && valid_token) {

                token[token_length] = '\0';

                add_term(
                    token,
                    book_id
                );
            }

            token_length = 0;
            valid_token = 1;
        }
    }


    if (token_length > 0 && valid_token) {

        token[token_length] = '\0';

        add_term(
            token,
            book_id
        );
    }


    fclose(file);
}


void save_index(void) {

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

void save_hierarchical_index(void) {

    _mkdir("datamart_c");

    _mkdir(
        "datamart_c/inverted_index_hierarchical"
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

            char letter[2];

            letter[0] =
                (char)toupper(
                    (unsigned char)current->term[0]
                );

            letter[1] =
                '\0';


            char folder[256];

            sprintf(
                folder,
                "datamart_c/inverted_index_hierarchical/%s",
                letter
            );


            _mkdir(folder);


            char path[512];

            sprintf(
                path,
                "%s/%s.txt",
                folder,
                current->term
            );


            FILE *file =
                fopen(
                    path,
                    "w"
                );


            if (
                file != NULL
            ) {

                for (
                    int j = 0;
                    j < current->book_count;
                    j++
                ) {

                    fprintf(
                        file,
                        "%d\n",
                        current->book_ids[j]
                    );
                }

                fclose(file);
            }


            current =
                current->next;
        }
    }


    printf(
        "Hierarchical index saved\n"
    );
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

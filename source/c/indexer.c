#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MAKE_DIR(path) mkdir(path, 0777)
#endif
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
        ||
        (unsigned char)c >= 0x80   // any UTF-8 continuation/lead byte
    );
}


char to_lowercase(char c) {

    if (c >= 'A' && c <= 'Z') {

        return c + ('a' - 'A');
    }

    return c;
}

unsigned int read_utf8_codepoint(FILE *file, int first_byte) {

    unsigned int codepoint;
    int extra_bytes;

    if (first_byte < 0x80) {
        return (unsigned int)first_byte;
    }

    if ((first_byte & 0xE0) == 0xC0) {
        codepoint = first_byte & 0x1F;
        extra_bytes = 1;
    }
    else if ((first_byte & 0xF0) == 0xE0) {
        codepoint = first_byte & 0x0F;
        extra_bytes = 2;
    }
    else if ((first_byte & 0xF8) == 0xF0) {
        codepoint = first_byte & 0x07;
        extra_bytes = 3;
    }
    else {
        return 0;
    }

    for (int i = 0; i < extra_bytes; i++) {

        int next = fgetc(file);

        if (next == EOF) {
            break;
        }

        codepoint =
            (codepoint << 6)
            |
            (next & 0x3F);
    }

    return codepoint;
}


int is_unicode_word_char(unsigned int cp) {

    /*
     * Letras latinas Unicode:
     * á, é, ï, æ, œ, etc.
     */
    if (
        (cp >= 0x00C0 && cp <= 0x00D6)
        ||
        (cp >= 0x00D8 && cp <= 0x00F6)
        ||
        (cp >= 0x00F8 && cp <= 0x024F)
    ) {
        return 1;
    }

    return 0;
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

    if (c < 128) {

        if (is_word_char((char)c)) {

            if (is_letter((char)c)) {

                if (token_length < MAX_TOKEN_SIZE - 1) {

                    token[token_length] =
                        to_lowercase((char)c);

                    token_length++;
                }
            }

            else {

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

    else {

        unsigned int cp =
            read_utf8_codepoint(
                file,
                c
            );

        if (is_unicode_word_char(cp)) {

            /*
             * Es una letra Unicode como œ, æ, é...
             *
             * Python la considera parte de la palabra,
             * pero no pertenece a [a-zA-Z].
             * Por tanto, la palabra entera deja de ser
             * un token válido.
             */
            valid_token = 0;
        }

        else {

            /*
             * Es puntuación Unicode, por ejemplo ’.
             * Python la considera separador.
             */

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

    MAKE_DIR("datamart_c");

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

    MAKE_DIR("datamart_c");

    MAKE_DIR(
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


            MAKE_DIR(folder);


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

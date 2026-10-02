#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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


void index_book(int book_id) {

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


void save_hierarchical_index() {

    mkdir(
        "datamart_c",
        0777
    );

    mkdir(
        "datamart_c/inverted_index_hierarchical",
        0777
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
                current->term[0];

            letter[1] =
                '\0';


            char folder[256];

            sprintf(
                folder,
                "datamart_c/inverted_index_hierarchical/%s",
                letter
            );


            mkdir(
                folder,
                0777
            );


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

                    if (j > 0) {

                        fprintf(
                            file,
                            " "
                        );
                    }

                    fprintf(
                        file,
                        "%d",
                        current->book_ids[j]
                    );
                }


                fprintf(
                    file,
                    "\n"
                );

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


void search_hierarchical(
    const char *term
) {

    char path[512];

    sprintf(
        path,
        "datamart_c/inverted_index_hierarchical/%c/%s.txt",
        term[0],
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


    if (
        file == NULL
    ) {

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


int main() {

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


    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define START_MARKER "*** START OF THE PROJECT GUTENBERG EBOOK"
#define END_MARKER "*** END OF THE PROJECT GUTENBERG EBOOK"


int save_book(
    int book_id,
    const char *header,
    const char *body,
    const char *strategy
);


typedef struct {
    char *data;
    size_t size;
} Memory;


size_t write_callback(
    void *contents,
    size_t size,
    size_t nmemb,
    void *userp
) {

    size_t total_size = size * nmemb;

    Memory *memory = (Memory *) userp;

    char *new_data = realloc(
        memory->data,
        memory->size + total_size + 1
    );

    if (new_data == NULL) {
        return 0;
    }

    memory->data = new_data;

    memcpy(
        &(memory->data[memory->size]),
        contents,
        total_size
    );

    memory->size += total_size;

    memory->data[memory->size] = '\0';

    return total_size;
}


char *download_book(int book_id) {

    CURL *curl;

    Memory memory;

    memory.data = malloc(1);
    memory.size = 0;

    char url[256];

    sprintf(
        url,
        "https://www.gutenberg.org/cache/epub/%d/pg%d.txt",
        book_id,
        book_id
    );

    curl = curl_easy_init();

    if (curl == NULL) {
        return NULL;
    }

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        write_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &memory
    );

    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        1L
    );

    CURLcode result =
        curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {

        free(memory.data);

        return NULL;
    }

    return memory.data;
}


int split_book(
    char *text,
    char **header,
    char **body
) {

    char *start =
        strstr(text, START_MARKER);

    char *end =
        strstr(text, END_MARKER);

    if (start == NULL || end == NULL) {

        printf(
            "Gutenberg markers not found\n"
        );

        return 0;
    }

    size_t header_size =
        start - text;

    *header = malloc(
        header_size + 1
    );

    strncpy(
        *header,
        text,
        header_size
    );

    (*header)[header_size] = '\0';


    start += strlen(START_MARKER);

    size_t body_size =
        end - start;

    *body = malloc(
        body_size + 1
    );

    strncpy(
        *body,
        start,
        body_size
    );

    (*body)[body_size] = '\0';

    return 1;
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

        int book_id = books[i];

        printf(
            "Downloading book %d...\n",
            book_id
        );

        char *text =
            download_book(book_id);

        if (text == NULL) {

            printf(
                "Error downloading book %d\n",
                book_id
            );

            continue;
        }


        char *header = NULL;
        char *body = NULL;

        if (
            split_book(
                text,
                &header,
                &body
            )
        ) {

            printf(
                "Book %d downloaded and split successfully\n",
                book_id
            );

            save_book(
                book_id,
                header,
                body,
                "time"
            );

            save_book(
                book_id,
                header,
                body,
                "book"
            );

            save_book(
                book_id,
                header,
                body,
                "batch"
            );


            printf(
                "Header size: %zu bytes\n",
                strlen(header)
            );

            printf(
                "Body size: %zu bytes\n\n",
                strlen(body)
            );
        }


        free(header);
        free(body);
        free(text);
    }

    return 0;
}
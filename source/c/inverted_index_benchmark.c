#include <stdio.h>
#include <time.h>
#include "indexer.h"


double get_time_seconds(void) {

    struct timespec ts;

    timespec_get(
        &ts,
        TIME_UTC
    );

    return ts.tv_sec
        + ts.tv_nsec / 1000000000.0;
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
        sizeof(books) / sizeof(books[0]);


    printf(
        "C INVERTED INDEX BENCHMARK\n\n"
    );


    reset_index();

    double start =
        get_time_seconds();


    for (
        int i = 0;
        i < number_of_books;
        i++
    ) {

        index_book(
            books[i]
        );
    }


    double end =
        get_time_seconds();


    printf(
        "Unique terms: %d\n",
        get_unique_term_count()
    );

    printf(
        "Index build time: %.6f seconds\n",
        end - start
    );


    reset_index();

    return 0;
}
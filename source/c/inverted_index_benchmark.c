#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include "indexer.h"
#include "mongodb_index.h"


#define QUERY_REPETITIONS 1000


double get_time_seconds(void) {

    struct timespec ts;

    timespec_get(
        &ts,
        TIME_UTC
    );

    return ts.tv_sec
        + ts.tv_nsec / 1000000000.0;
}


void build_books(
    int books[],
    int number_of_books
) {

    for (
        int i = 0;
        i < number_of_books;
        i++
    ) {

        index_book(
            books[i]
        );
    }
}



void benchmark_monolithic_build(
    int books[],
    int number_of_books
) {

    printf(
        "\nMONOLITHIC BUILD BENCHMARK\n"
    );

    reset_index();

    double start =
        get_time_seconds();


    build_books(
        books,
        number_of_books
    );

    save_index();


    double end =
        get_time_seconds();


    printf(
        "Unique terms: %d\n",
        get_unique_term_count()
    );

    printf(
        "Build time: %.6f seconds\n",
        end - start
    );
}


void benchmark_hierarchical_build(
    int books[],
    int number_of_books
) {

    printf(
        "\nHIERARCHICAL BUILD BENCHMARK\n"
    );

    reset_index();

    double start =
        get_time_seconds();


    build_books(
        books,
        number_of_books
    );

    save_hierarchical_index();


    double end =
        get_time_seconds();


    printf(
        "Unique terms: %d\n",
        get_unique_term_count()
    );

    printf(
        "Build time: %.6f seconds\n",
        end - start
    );
}


void benchmark_mongodb_build(
    int books[],
    int number_of_books
) {

    printf(
        "\nMONGODB BUILD BENCHMARK\n"
    );


    reset_index();


    double start =
        get_time_seconds();


    build_books(
        books,
        number_of_books
    );


    int result =
        save_mongodb_index();


    double end =
        get_time_seconds();


    if (result != 0) {

        printf(
            "Error saving MongoDB index\n"
        );

        return;
    }


    printf(
        "Unique terms: %d\n",
        get_unique_term_count()
    );


    printf(
        "Build time: %.6f seconds\n",
        end - start
    );
}


void benchmark_monolithic_queries(void) {

    printf(
        "\nMONOLITHIC QUERY BENCHMARK\n"
    );


    const char *terms[] = {
        "pride",
        "monster",
        "nonexistentword"
    };


    int number_of_terms = 3;


    for (
        int i = 0;
        i < number_of_terms;
        i++
    ) {

        double start =
            get_time_seconds();


        int result = 0;


        for (
            int j = 0;
            j < QUERY_REPETITIONS;
            j++
        ) {

            result =
                query_term_count(
                    terms[i]
                );
        }


        double end =
            get_time_seconds();


        double total =
            end - start;


        printf(
            "\nTerm: %s\n",
            terms[i]
        );

        printf(
            "Matches: %d\n",
            result
        );

        printf(
            "Total time: %.6f seconds\n",
            total
        );

        printf(
            "Average: %.9f seconds\n",
            total / QUERY_REPETITIONS
        );
    }
}


void benchmark_hierarchical_queries(void) {

    printf(
        "\nHIERARCHICAL QUERY BENCHMARK\n"
    );


    const char *terms[] = {
        "pride",
        "monster",
        "nonexistentword"
    };


    int number_of_terms = 3;


    for (
        int i = 0;
        i < number_of_terms;
        i++
    ) {

        double start =
            get_time_seconds();


        int result = 0;


        for (
            int j = 0;
            j < QUERY_REPETITIONS;
            j++
        ) {

            result =
                query_hierarchical_count(
                    terms[i]
                );
        }


        double end =
            get_time_seconds();


        double total =
            end - start;


        printf(
            "\nTerm: %s\n",
            terms[i]
        );

        printf(
            "Matches: %d\n",
            result
        );

        printf(
            "Total time: %.6f seconds\n",
            total
        );

        printf(
            "Average: %.9f seconds\n",
            total / QUERY_REPETITIONS
        );
    }
}


int query_mongodb_count(
    mongoc_collection_t *collection,
    const char *term
) {

    bson_t query;

    const bson_t *document;

    mongoc_cursor_t *cursor;


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


    int count = 0;


    if (
        mongoc_cursor_next(
            cursor,
            &document
        )
    ) {

        bson_iter_t iterator;


        if (
            bson_iter_init_find(
                &iterator,
                document,
                "postings"
            )
            &&
            BSON_ITER_HOLDS_ARRAY(
                &iterator
            )
        ) {

            bson_iter_t array_iterator;


            if (
                bson_iter_recurse(
                    &iterator,
                    &array_iterator
                )
            ) {

                while (
                    bson_iter_next(
                        &array_iterator
                    )
                ) {

                    count++;
                }
            }
        }
    }


    mongoc_cursor_destroy(
        cursor
    );

    bson_destroy(
        &query
    );


    return count;
}


void benchmark_mongodb_queries(void) {

    printf(
        "\nMONGODB QUERY BENCHMARK\n"
    );


    const char *terms[] = {
        "pride",
        "monster",
        "nonexistentword"
    };


    int number_of_terms = 3;


    mongoc_client_t *client =
        mongoc_client_new(
            "mongodb://localhost:27017"
        );


    mongoc_collection_t *collection =
        mongoc_client_get_collection(
            client,
            "stage1",
            "inverted_index"
        );


    for (
        int i = 0;
        i < number_of_terms;
        i++
    ) {

        double start =
            get_time_seconds();


        int result = 0;


        for (
            int j = 0;
            j < QUERY_REPETITIONS;
            j++
        ) {

            result =
                query_mongodb_count(
                    collection,
                    terms[i]
                );
        }


        double end =
            get_time_seconds();


        double total =
            end - start;


        printf(
            "\nTerm: %s\n",
            terms[i]
        );

        printf(
            "Matches: %d\n",
            result
        );

        printf(
            "Total time: %.6f seconds\n",
            total
        );

        printf(
            "Average: %.9f seconds\n",
            total / QUERY_REPETITIONS
        );
    }


    mongoc_collection_destroy(
        collection
    );

    mongoc_client_destroy(
        client
    );
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
        "C INVERTED INDEX BENCHMARK\n"
    );



    benchmark_monolithic_build(
        books,
        number_of_books
    );

    benchmark_monolithic_queries();


    benchmark_hierarchical_build(
        books,
        number_of_books
    );

    benchmark_hierarchical_queries();

    mongoc_init();


    benchmark_mongodb_build(
        books,
        number_of_books
    );
    
    benchmark_mongodb_queries();


    mongoc_cleanup();


    reset_index();


    return 0;
}
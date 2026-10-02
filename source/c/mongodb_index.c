#include <stdio.h>

#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include "indexer.h"
#include "mongodb_index.h"


typedef struct {

    mongoc_collection_t *collection;

    int errors;

} MongoContext;


static void insert_term_mongodb(
    const char *term,
    const int *book_ids,
    int book_count,
    void *context
) {

    MongoContext *mongo_context =
        (MongoContext *)context;


    bson_t document;
    bson_t array;

    bson_error_t error;


    bson_init(
        &document
    );


    BSON_APPEND_UTF8(
        &document,
        "term",
        term
    );


    BSON_APPEND_ARRAY_BEGIN(
        &document,
        "postings",
        &array
    );


    for (
        int i = 0;
        i < book_count;
        i++
    ) {

        char key[16];

        snprintf(
            key,
            sizeof(key),
            "%d",
            i
        );


        BSON_APPEND_INT32(
            &array,
            key,
            book_ids[i]
        );
    }


    bson_append_array_end(
        &document,
        &array
    );


    if (
        !mongoc_collection_insert_one(
            mongo_context->collection,
            &document,
            NULL,
            NULL,
            &error
        )
    ) {

        mongo_context->errors++;
    }


    bson_destroy(
        &document
    );
}


int save_mongodb_index(void) {

    mongoc_client_t *client =
        mongoc_client_new(
            "mongodb://localhost:27017"
        );


    if (client == NULL) {
        return -1;
    }


    mongoc_collection_t *collection =
        mongoc_client_get_collection(
            client,
            "stage1",
            "inverted_index"
        );


    bson_t empty =
        BSON_INITIALIZER;

    bson_error_t error;


    if (
        !mongoc_collection_delete_many(
            collection,
            &empty,
            NULL,
            NULL,
            &error
        )
    ) {

        mongoc_collection_destroy(
            collection
        );

        mongoc_client_destroy(
            client
        );

        return -1;
    }


    MongoContext context;

    context.collection =
        collection;

    context.errors = 0;


    for_each_term(
        insert_term_mongodb,
        &context
    );


    mongoc_collection_destroy(
        collection
    );

    mongoc_client_destroy(
        client
    );


    if (context.errors > 0) {
        return -1;
    }


    return 0;
}
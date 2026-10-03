#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>


#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <dirent.h>
#endif


#include <mongoc/mongoc.h>
#include <bson/bson.h>


#include "indexer.h"
#include "mongodb_index.h"


/* =========================================================
   Common helpers
   ========================================================= */

static double get_time_seconds(void) {

    struct timespec ts;

    timespec_get(
        &ts,
        TIME_UTC
    );

    return ts.tv_sec
        + ts.tv_nsec / 1000000000.0;
}


static void build_books(
    const int *books,
    int count
) {

    for (int i = 0; i < count; i++) {

        index_book(
            books[i]
        );
    }
}


/* =========================================================
   Hierarchical cleanup
   ========================================================= */

static void clear_hierarchical_index(void) {

#ifdef _WIN32

    system(
        "if exist datamart_c\\inverted_index_hierarchical "
        "rmdir /S /Q datamart_c\\inverted_index_hierarchical"
    );

#else

    system(
        "rm -rf datamart_c/inverted_index_hierarchical"
    );

#endif
}


/* =========================================================
   Disk usage
   ========================================================= */

static long file_size(
    const char *path
) {

    struct stat info;

    if (
        stat(
            path,
            &info
        ) != 0
    ) {

        return -1;
    }

    return (long)info.st_size;
}


#ifdef _WIN32

static long folder_size(
    const char *root
) {

    char search_path[512];

    snprintf(
        search_path,
        sizeof(search_path),
        "%s\\*",
        root
    );


    WIN32_FIND_DATAA find_data;

    HANDLE handle =
        FindFirstFileA(
            search_path,
            &find_data
        );


    long total = 0;


    if (
        handle ==
        INVALID_HANDLE_VALUE
    ) {

        return 0;
    }


    do {

        if (
            strcmp(
                find_data.cFileName,
                "."
            ) == 0
            ||
            strcmp(
                find_data.cFileName,
                ".."
            ) == 0
        ) {

            continue;
        }


        char full_path[512];

        snprintf(
            full_path,
            sizeof(full_path),
            "%s\\%s",
            root,
            find_data.cFileName
        );


        if (
            find_data.dwFileAttributes
            & FILE_ATTRIBUTE_DIRECTORY
        ) {

            total +=
                folder_size(
                    full_path
                );

        } else {

            LARGE_INTEGER size;

            size.LowPart =
                find_data.nFileSizeLow;

            size.HighPart =
                find_data.nFileSizeHigh;

            total +=
                (long)size.QuadPart;
        }

    } while (
        FindNextFileA(
            handle,
            &find_data
        )
    );


    FindClose(handle);

    return total;
}

#else

static long folder_size(
    const char *root
) {

    DIR *dir =
        opendir(root);


    if (dir == NULL) {
        return 0;
    }


    long total = 0;

    struct dirent *entry;


    while (
        (entry = readdir(dir))
        != NULL
    ) {

        if (
            strcmp(
                entry->d_name,
                "."
            ) == 0
            ||
            strcmp(
                entry->d_name,
                ".."
            ) == 0
        ) {

            continue;
        }


        char full_path[512];

        snprintf(
            full_path,
            sizeof(full_path),
            "%s/%s",
            root,
            entry->d_name
        );


        struct stat info;

        if (
            stat(
                full_path,
                &info
            ) != 0
        ) {

            continue;
        }


        if (
            S_ISDIR(
                info.st_mode
            )
        ) {

            total +=
                folder_size(
                    full_path
                );

        } else {

            total +=
                (long)info.st_size;
        }
    }


    closedir(dir);

    return total;
}

#endif


static long mongodb_disk_size(
    mongoc_client_t *client
) {

    mongoc_database_t *db =
        mongoc_client_get_database(
            client,
            "stage1"
        );


    bson_t *cmd =
        BCON_NEW(
            "collStats",
            BCON_UTF8(
                "inverted_index"
            )
        );


    bson_t reply;

    bson_error_t error;


    long total = -1;


    if (
        mongoc_database_command_simple(
            db,
            cmd,
            NULL,
            &reply,
            &error
        )
    ) {

        bson_iter_t iter;

        long storage_size = 0;
        long index_size = 0;


        if (
            bson_iter_init_find(
                &iter,
                &reply,
                "storageSize"
            )
        ) {

            storage_size =
                (long)bson_iter_as_int64(
                    &iter
                );
        }


        if (
            bson_iter_init_find(
                &iter,
                &reply,
                "totalIndexSize"
            )
        ) {

            index_size =
                (long)bson_iter_as_int64(
                    &iter
                );
        }


        total =
            storage_size
            + index_size;

    } else {

        printf(
            "collStats failed: %s\n",
            error.message
        );
    }


    bson_destroy(&reply);

    bson_destroy(cmd);

    mongoc_database_destroy(db);


    return total;
}


static void benchmark_disk_usage(
    mongoc_client_t *client
) {

    printf(
        "\nDISK USAGE BENCHMARK\n"
    );


    long json_size =
        file_size(
            "datamart_c/inverted_index.json"
        );


    long hierarchical_size =
        folder_size(
            "datamart_c/inverted_index_hierarchical"
        );


    long mongo_size =
        mongodb_disk_size(
            client
        );


    if (json_size >= 0) {

        printf(
            "Monolithic JSON: %.2f KB\n",
            json_size / 1024.0
        );

    } else {

        printf(
            "Monolithic JSON: file not found\n"
        );
    }


    printf(
        "Hierarchical index: %.2f KB\n",
        hierarchical_size / 1024.0
    );


    if (mongo_size >= 0) {

        printf(
            "MongoDB: %.2f KB\n",
            mongo_size / 1024.0
        );

    } else {

        printf(
            "MongoDB: could not read collStats\n"
        );
    }
}


/* =========================================================
   Memory usage
   ========================================================= */

static void benchmark_memory_usage(void) {

    printf(
        "\nMEMORY USAGE BENCHMARK\n"
    );


    long bytes =
        get_index_memory_usage();


    printf(
        "Monolithic in-memory index: %.2f KB\n",
        bytes / 1024.0
    );


    printf(
        "Hierarchical: client-side resident memory "
        "not measured separately\n"
    );


    printf(
        "MongoDB: client-side memory only; "
        "server memory excluded\n"
    );
}


/* =========================================================
   Update performance
   ========================================================= */

static void benchmark_update_performance(
    const int *all_books,
    int total_count
) {

    printf(
        "\nUPDATE PERFORMANCE BENCHMARK\n"
    );


    /*
     * Initial state:
     * build the index using every book
     * except the final one.
     */

    reset_index();


    build_books(
        all_books,
        total_count - 1
    );


    save_index();


    clear_hierarchical_index();

    save_hierarchical_index();


    save_mongodb_index();


    /*
     * Last book is considered the new
     * incremental update.
     */

    int new_book_id =
        all_books[
            total_count - 1
        ];


    /*
     * Tokenize the new book once.
     * This preprocessing is shared by
     * all three storage structures.
     */

    double start =
        get_time_seconds();


    index_book(
        new_book_id
    );


    double preprocessing_time =
        get_time_seconds()
        - start;


    printf(
        "New book preprocessing: %.6f seconds\n",
        preprocessing_time
    );


    /*
     * Monolithic:
     * complete JSON must be rewritten.
     */

    start =
        get_time_seconds();


    save_index();


    printf(
        "Monolithic index update: %.6f seconds\n",
        get_time_seconds()
        - start
    );


    /*
     * Hierarchical:
     * only affected term files
     * are updated.
     */

    start =
        get_time_seconds();


    update_hierarchical_index_for_book(
        new_book_id
    );


    printf(
        "Hierarchical index update: %.6f seconds\n",
        get_time_seconds()
        - start
    );


    /*
     * MongoDB:
     * only affected term documents
     * are updated.
     */

    start =
        get_time_seconds();


    update_mongodb_index_for_book(
        new_book_id
    );


    printf(
        "MongoDB index update: %.6f seconds\n",
        get_time_seconds()
        - start
    );
}


/* =========================================================
   Scalability
   ========================================================= */

static void benchmark_scalability(
    const int *all_books
) {

    printf(
        "\nSCALABILITY BENCHMARK\n"
    );


    int dataset_sizes[] = {
        1,
        3,
        5,
        6
    };


    int number_of_sizes =
        sizeof(dataset_sizes)
        /
        sizeof(dataset_sizes[0]);


    for (
        int d = 0;
        d < number_of_sizes;
        d++
    ) {

        int count =
            dataset_sizes[d];


        printf(
            "\nBooks: %d\n",
            count
        );


        reset_index();


        /*
         * Common logical index build.
         */

        double start =
            get_time_seconds();


        build_books(
            all_books,
            count
        );


        double build_time =
            get_time_seconds()
            - start;


        printf(
            "Unique terms: %d\n",
            get_unique_term_count()
        );


        printf(
            "Build time: %.6f seconds\n",
            build_time
        );


        /*
         * Monolithic storage.
         */

        start =
            get_time_seconds();


        save_index();


        printf(
            "Monolithic storage: %.6f seconds\n",
            get_time_seconds()
            - start
        );


        /*
         * Hierarchical storage.
         * Previous files must be deleted
         * before every dataset size.
         */

        clear_hierarchical_index();


        start =
            get_time_seconds();


        save_hierarchical_index();


        printf(
            "Hierarchical storage: %.6f seconds\n",
            get_time_seconds()
            - start
        );


        /*
         * MongoDB storage.
         */

        start =
            get_time_seconds();


        save_mongodb_index();


        printf(
            "MongoDB storage: %.6f seconds\n",
            get_time_seconds()
            - start
        );
    }
}


/* =========================================================
   Main
   ========================================================= */

int main(void) {

    int books[] = {
        1342,
        11,
        84,
        98,
        1661,
        2701
    };


    int number_of_books =
        sizeof(books)
        /
        sizeof(books[0]);


    printf(
        "C INVERTED INDEX RESOURCE BENCHMARK\n"
    );


    mongoc_init();


    mongoc_client_t *client =
        mongoc_client_new(
            "mongodb://localhost:27017"
        );


    if (client == NULL) {

        printf(
            "Could not create MongoDB client\n"
        );

        mongoc_cleanup();

        return 1;
    }


    /*
     * Build and store the complete
     * six-book dataset once so disk
     * and memory measurements refer
     * to a known complete state.
     */

    reset_index();


    build_books(
        books,
        number_of_books
    );


    save_index();


    clear_hierarchical_index();

    save_hierarchical_index();


    save_mongodb_index();


    /*
     * Resource benchmarks.
     */

    benchmark_disk_usage(
        client
    );


    benchmark_memory_usage();


    benchmark_update_performance(
        books,
        number_of_books
    );


    benchmark_scalability(
        books
    );


    mongoc_client_destroy(
        client
    );


    mongoc_cleanup();


    reset_index();


    return 0;
}
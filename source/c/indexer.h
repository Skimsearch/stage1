#ifndef INDEXER_H
#define INDEXER_H

typedef void (*term_callback)(
    const char *term,
    const int *book_ids,
    int book_count,
    void *context
);

void index_book(int book_id);
void save_index(void);
void save_hierarchical_index(void);

int query_term_count(const char *term);
int query_hierarchical_count(const char *term);

void reset_index(void);

int get_unique_term_count(void);

typedef void (*term_callback)(
    const char *term,
    const int *book_ids,
    int book_count,
    void *context
);

void for_each_term(
    term_callback callback,
    void *context
);

#endif
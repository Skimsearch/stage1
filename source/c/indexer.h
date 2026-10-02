#ifndef INDEXER_H
#define INDEXER_H

void index_book(int book_id);
void save_index(void);

int query_term_count(const char *term);

void reset_index(void);

int get_unique_term_count(void);
void save_hierarchical_index(void);

int query_hierarchical_count(const char *term);
#endif
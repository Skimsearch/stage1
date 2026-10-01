#ifndef INDEXER_H
#define INDEXER_H

void index_book(int book_id);
void save_index(void);

int query_term_count(const char *term);

void reset_index(void);

#endif
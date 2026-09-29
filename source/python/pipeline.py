from control import (
    control_pipeline_step,
    get_downloaded_books,
    mark_indexed
)

from downloader import download_book

from inverted_index import (
    build_inverted_index,
    load_inverted_index,
    save_inverted_index
)

from hierarchical_index import (
    update_hierarchical_index
)

from inverted_index_mongodb import (
    update_mongodb_index
)

DOWNLOAD_CANDIDATES = [
    74,
    76,
    174,
    345,
    1260,
    1952
]

def choose_new_book():

    downloaded = get_downloaded_books()

    for book_id in DOWNLOAD_CANDIDATES:

        if book_id not in downloaded:
            return book_id

    return None

def run_pipeline_step():

    action, book_id = control_pipeline_step()

    if action == "index":

        print(
            f"[PIPELINE] Indexing book "
            f"{book_id}"
        )

        new_index = build_inverted_index(
            [book_id]
        )

        if not new_index:

            print(
                f"[PIPELINE] Could not index "
                f"book {book_id}"
            )

            return

        # Update monolithic index
        current_index = load_inverted_index()

        for term, book_ids in new_index.items():

            if term not in current_index:
                current_index[term] = []

            for indexed_book_id in book_ids:

                if indexed_book_id not in current_index[term]:
                    current_index[term].append(
                        indexed_book_id
                    )

        save_inverted_index(current_index)

        # Update hierarchical index
        update_hierarchical_index(
            new_index
        )

        # Update MongoDB index
        update_mongodb_index(
            new_index
        )

        # Only mark the book after indexing succeeds
        mark_indexed(book_id)

        print(
            f"[PIPELINE] Book "
            f"{book_id} successfully indexed"
        )

    elif action == "download":

        new_book_id = choose_new_book()

        if new_book_id is None:

            print(
                "[PIPELINE] No new books available"
            )

            return

        print(
            f"[PIPELINE] Downloading book "
            f"{new_book_id}"
        )

        success = download_book(
            new_book_id,
            "book"
        )

        if success:

            print(
                f"[PIPELINE] Book "
                f"{new_book_id} downloaded"
            )

    else:

        raise ValueError(
            f"Unknown action: {action}"
        )


if __name__ == "__main__":

    run_pipeline_step()
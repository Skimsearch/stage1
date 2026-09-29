from control import (
    control_pipeline_step,
    get_downloaded_books
)

from downloader import download_book

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
            f"[PIPELINE] Next action: "
            f"index book {book_id}"
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
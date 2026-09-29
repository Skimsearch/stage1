from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]

CONTROL_PATH = PROJECT_ROOT / "control"

DOWNLOADED_FILE = CONTROL_PATH / "downloaded_books.txt"
INDEXED_FILE = CONTROL_PATH / "indexed_books.txt"


def setup_control():

    CONTROL_PATH.mkdir(
        parents=True,
        exist_ok=True
    )

    DOWNLOADED_FILE.touch(
        exist_ok=True
    )

    INDEXED_FILE.touch(
        exist_ok=True
    )


def read_books(file_path):

    setup_control()

    books = file_path.read_text(
        encoding="utf-8"
    ).splitlines()

    return {
        int(book_id)
        for book_id in books
        if book_id.strip()
    }


def add_book(file_path, book_id):

    setup_control()

    books = read_books(file_path)

    if book_id not in books:

        with open(
            file_path,
            "a",
            encoding="utf-8"
        ) as file:

            file.write(f"{book_id}\n")


def get_downloaded_books():

    return read_books(
        DOWNLOADED_FILE
    )


def get_indexed_books():

    return read_books(
        INDEXED_FILE
    )


def mark_downloaded(book_id):

    add_book(
        DOWNLOADED_FILE,
        book_id
    )

def get_pending_books():

    downloaded = get_downloaded_books()
    indexed = get_indexed_books()

    return downloaded - indexed


def mark_indexed(book_id):

    add_book(
        INDEXED_FILE,
        book_id
    )

def control_pipeline_step():

    pending_books = get_pending_books()

    if pending_books:

        book_id = min(pending_books)

        print(
            f"[CONTROL] Book {book_id} ready to be indexed"
        )

        return "index", book_id

    print(
        "[CONTROL] No books pending for indexing"
    )

    return "download", None

#Temporally
if __name__ == "__main__":

    setup_control()

    print("Downloaded:", get_downloaded_books())
    print("Indexed:", get_indexed_books())
    print("Pending:", get_pending_books())

    action, book_id = control_pipeline_step()

    print("Action:", action)
    print("Book:", book_id)
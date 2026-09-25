import requests
from pathlib import Path
from datetime import datetime


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DATALAKE_PATH = PROJECT_ROOT / "datalake"


START_MARKER = "*** START OF THE PROJECT GUTENBERG EBOOK"
END_MARKER = "*** END OF THE PROJECT GUTENBERG EBOOK"


def get_output_path(book_id: int, strategy: str, when: datetime = None):
    if strategy == "time":
        when = when or datetime.now()

        return (
            DATALAKE_PATH
            / "time"
            / when.strftime("%Y%m%d")
            / when.strftime("%H")
        )

    elif strategy == "book":
        return (
            DATALAKE_PATH
            / "book"
            / str(book_id)
        )

    elif strategy == "batch":
        batch_start = (book_id // 1000) * 1000
        batch_end = batch_start + 999

        return (
            DATALAKE_PATH
            / "batch"
            / f"{batch_start}-{batch_end}"
        )

    else:
        raise ValueError(f"Unknown strategy: {strategy}")


def fetch_book(book_id: int):
    """Network-only step: download and split the raw text. Returns
    (header, body) or None if the markers weren't found. No files are
    written here."""
    url = f"https://www.gutenberg.org/cache/epub/{book_id}/pg{book_id}.txt"

    response = requests.get(url, timeout=20)
    response.raise_for_status()

    text = response.text

    if START_MARKER not in text or END_MARKER not in text:
        print("Gutenberg markers not found")
        return None

    header, body_and_footer = text.split(START_MARKER, 1)
    body, footer = body_and_footer.split(END_MARKER, 1)

    return header.strip(), body.strip()


def save_book(book_id: int, header: str, body: str, strategy: str, when: datetime = None):
    """Disk-only step: write already-downloaded header/body to the
    right location for the given strategy. No network call here."""
    output_path = get_output_path(book_id, strategy, when)
    output_path.mkdir(parents=True, exist_ok=True)

    header_path = output_path / f"{book_id}.header.txt"
    body_path = output_path / f"{book_id}.body.txt"

    header_path.write_text(header, encoding="utf-8")
    body_path.write_text(body, encoding="utf-8")


def download_book(book_id: int, strategy: str):
    """Kept for convenience / backwards compatibility: fetch + save in
    one call. Benchmarks that need to separate network from disk
    should call fetch_book/save_book directly instead."""
    result = fetch_book(book_id)
    if result is None:
        return False

    header, body = result
    save_book(book_id, header, body, strategy)

    print(f"Book {book_id} successfully downloaded")

    return True


def download_books(book_ids: list[int], strategy: str):
    for book_id in book_ids:
        download_book(book_id, strategy)


if __name__ == "__main__":

    books = [1342, 11, 84, 98, 1661]

    download_books(books, "time")
    download_books(books, "book")
    download_books(books, "batch")
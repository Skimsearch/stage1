import re
import sqlite3
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]
DATALAKE_PATH = PROJECT_ROOT / "datalake"
DATAMART_PATH = PROJECT_ROOT / "datamart"


def extract_metadata(header_path, book_id):

    text = header_path.read_text(encoding="utf-8")

    title_match = re.search(r"^Title:\s*(.+)$", text, re.MULTILINE)
    author_match = re.search(r"^Author:\s*(.+)$", text, re.MULTILINE)
    language_match = re.search(r"^Language:\s*(.+)$", text, re.MULTILINE)

    title = title_match.group(1).strip() if title_match else "Unknown"
    author = author_match.group(1).strip() if author_match else "Unknown"
    language = language_match.group(1).strip() if language_match else "Unknown"

    return book_id, title, author, language


def save_metadata(metadata):

    DATAMART_PATH.mkdir(parents=True, exist_ok=True)

    db_path = DATAMART_PATH / "metadata.db"

    connection = sqlite3.connect(db_path)
    cursor = connection.cursor()

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS books (
            book_id INTEGER PRIMARY KEY,
            title TEXT,
            author TEXT,
            language TEXT
        )
    """)

    cursor.execute("""
        INSERT OR REPLACE INTO books
        (book_id, title, author, language)
        VALUES (?, ?, ?, ?)
    """, metadata)

    connection.commit()
    connection.close()


if __name__ == "__main__":

    books = [1342, 11, 84, 98, 1661]

    for book_id in books:

        header_path = (
            DATALAKE_PATH
            / "book"
            / str(book_id)
            / f"{book_id}.header.txt"
        )

        metadata = extract_metadata(header_path, book_id)

        save_metadata(metadata)

        print(metadata)
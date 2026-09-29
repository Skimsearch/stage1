from pathlib import Path

from inverted_index import build_inverted_index

from control import mark_indexed

PROJECT_ROOT = Path(__file__).resolve().parents[2]

HIERARCHICAL_PATH = (
    PROJECT_ROOT
    / "datamart"
    / "inverted_index_hierarchical"
)


def save_hierarchical_index(inverted_index):

    HIERARCHICAL_PATH.mkdir(
        parents=True,
        exist_ok=True
    )

    for term, book_ids in inverted_index.items():

        letter = term[0].upper()

        folder = HIERARCHICAL_PATH / letter
        folder.mkdir(parents=True, exist_ok=True)

        term_path = folder / f"{term}.txt"

        with open(term_path, "w", encoding="utf-8") as file:
            for book_id in book_ids:
                file.write(f"{book_id}\n")

def search_hierarchical(term):

    term = term.lower()

    if not term:
        return []

    letter = term[0].upper()

    term_path = (
        HIERARCHICAL_PATH
        / letter
        / f"{term}.txt"
    )

    if not term_path.exists():
        return []

    book_ids = term_path.read_text(
        encoding="utf-8"
    ).splitlines()

    return [int(book_id) for book_id in book_ids]

def update_hierarchical_index(inverted_index):

    for term, book_ids in inverted_index.items():

        letter = term[0].upper()

        folder = HIERARCHICAL_PATH / letter
        folder.mkdir(parents=True, exist_ok=True)

        term_path = folder / f"{term}.txt"

        existing_ids = set()

        if term_path.exists():

            existing_ids = {
                int(book_id)
                for book_id in term_path.read_text(
                    encoding="utf-8"
                ).splitlines()
            }

        existing_ids.update(book_ids)

        with open(term_path, "w", encoding="utf-8") as file:

            for book_id in sorted(existing_ids):

                file.write(f"{book_id}\n")

if __name__ == "__main__":

    books = [1342, 11, 84, 98, 1661, 2701]

    inverted_index = build_inverted_index(books)

    save_hierarchical_index(inverted_index)

    for book_id in books:
        mark_indexed(book_id)

    print(
        f"Hierarchical index saved to: "
        f"{HIERARCHICAL_PATH}"
    )
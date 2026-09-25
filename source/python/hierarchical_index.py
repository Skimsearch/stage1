from pathlib import Path

from inverted_index import build_inverted_index


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

if __name__ == "__main__":

    books = [1342, 11, 84, 98, 1661]

    inverted_index = build_inverted_index(books)

    save_hierarchical_index(inverted_index)

    print(
        f"Hierarchical index saved to: "
        f"{HIERARCHICAL_PATH}"
    )

    print("\nSearch examples:")
    print("pride:", search_hierarchical("pride"))
    print("monster:", search_hierarchical("monster"))
    print("nonexistentword:", search_hierarchical("nonexistentword"))
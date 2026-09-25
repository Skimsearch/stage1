import json
from pathlib import Path

from tokenizer import tokenize


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DATALAKE_PATH = PROJECT_ROOT / "datalake"
INDEX_PATH = PROJECT_ROOT / "datamart" / "inverted_index"


def build_inverted_index(book_ids):

    inverted_index = {}

    for book_id in book_ids:

        body_path = (
            DATALAKE_PATH
            / "book"
            / str(book_id)
            / f"{book_id}.body.txt"
        )

        if not body_path.exists():
            print(f"Body for book {book_id} not found")
            continue

        text = body_path.read_text(encoding="utf-8")

        tokens = tokenize(text)

        # We only need each word once per book
        unique_tokens = set(tokens)

        for token in unique_tokens:

            if token not in inverted_index:
                inverted_index[token] = []

            inverted_index[token].append(book_id)

    return inverted_index

def save_inverted_index(inverted_index):

    INDEX_PATH.mkdir(parents=True, exist_ok=True)

    output_path = INDEX_PATH / "inverted_index.json"

    with open(output_path, "w", encoding="utf-8") as file:
        json.dump(
            inverted_index,
            file,
            ensure_ascii=False,
            indent=2
        )

    return output_path

def search_term(inverted_index, term):
    term = term.lower()
    return inverted_index.get(term, [])

if __name__ == "__main__":

    books = [1342, 11, 84, 98, 1661]

    inverted_index = build_inverted_index(books)

    output_path = save_inverted_index(inverted_index)

    print(f"Unique terms: {len(inverted_index)}")
    print(f"Index saved to: {output_path}")

    print("\nSearch examples:")
    print("pride:", search_term(inverted_index, "pride"))
    print("monster:", search_term(inverted_index, "monster"))
    print("nonexistentword:", search_term(inverted_index, "nonexistentword"))

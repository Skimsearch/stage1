import re
from pathlib import Path


def tokenize(text: str):

    text = text.lower()

    tokens = re.findall(r"\b[a-zA-Z]+\b", text)

    return tokens


if __name__ == "__main__":

    PROJECT_ROOT = Path(__file__).resolve().parents[2]

    body_path = (
        PROJECT_ROOT
        / "datalake"
        / "book"
        / "1342"
        / "1342.body.txt"
    )

    text = body_path.read_text(encoding="utf-8")

    tokens = tokenize(text)

    print(f"Total tokens: {len(tokens)}")
    print("First 20 tokens:")
    print(tokens[:20])
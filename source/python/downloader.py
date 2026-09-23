import requests
from pathlib import Path
from datetime import datetime

START_MARKER = "*** START OF THE PROJECT GUTENBERG EBOOK"
END_MARKER = "*** END OF THE PROJECT GUTENBERG EBOOK"


def get_output_path(book_id: int, strategy: str):
    if strategy == "time":
        now = datetime.now()

        return (
            Path("datalake")
            / "time"
            / now.strftime("%Y%m%d")
            / now.strftime("%H")
        )

    elif strategy == "book":
        return (
            Path("datalake")
            / "book"
            / str(book_id)
        )

    elif strategy == "batch":
        batch_start = (book_id // 1000) * 1000
        batch_end = batch_start + 999

        return (
            Path("datalake")
            / "batch"
            / f"{batch_start}-{batch_end}"
        )

    else:
        raise ValueError(f"Estrategia desconocida: {strategy}")
    
    
def download_book(book_id: int, strategy: str):
    url = f"https://www.gutenberg.org/cache/epub/{book_id}/pg{book_id}.txt"

    response = requests.get(url, timeout=20)
    response.raise_for_status()

    text = response.text

    if START_MARKER not in text or END_MARKER not in text:
        print("No se encontraron los marcadores de Gutenberg")
        return False

    header, body_and_footer = text.split(START_MARKER, 1)
    body, footer = body_and_footer.split(END_MARKER, 1)

    output_path = get_output_path(book_id, strategy)

    output_path.mkdir(parents=True, exist_ok=True)

    header_path = output_path / f"{book_id}.header.txt"
    body_path = output_path / f"{book_id}.body.txt"

    header_path.write_text(header.strip(), encoding="utf-8")
    body_path.write_text(body.strip(), encoding="utf-8")

    print(f"Libro {book_id} descargado correctamente")
    print(f"Header: {header_path}")
    print(f"Body: {body_path}")

    return True


if __name__ == "__main__":
    download_book(1342, "book")
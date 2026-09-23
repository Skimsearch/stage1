import time
from downloader import download_books

books = [1342, 11, 84, 98, 1661]

strategies = ["time", "book", "batch"]

for strategy in strategies:

    start = time.perf_counter()

    download_books(books, strategy)

    end = time.perf_counter()

    total_time = end - start

    print(f"\nEstrategia: {strategy}")
    print(f"  Tiempo total: {total_time:.2f} s")
    print(f"  Libros: {len(books)}")
    print(f"  Media por libro: {total_time / len(books):.2f} s")


def find_book(book_id, strategy):
    if strategy == "book":
        path = (
            Path("datalake/book")
            / str(book_id)
            / f"{book_id}.body.txt"
        )
        return path.exists()

    elif strategy == "batch":
        batch_start = (book_id // 1000) * 1000
        batch_end = batch_start + 999
        path = (
            Path("datalake/batch")
            / f"{batch_start}-{batch_end}"
            / f"{book_id}.body.txt"
        )
        return path.exists()

    elif strategy == "time":

        matches = list(
            Path("datalake/time").rglob(
                f"{book_id}.body.txt"
            )
        )
        return len(matches) > 0
print("\nBENCHMARK DE BÚSQUEDA")

book_id = 1342

for strategy in strategies:

    start = time.perf_counter()

    found = find_book(book_id, strategy)

    end = time.perf_counter()

    lookup_time = end - start

    print(f"\n{strategy}:")
    print(f"Tiempo de búsqueda: {lookup_time:.6f} segundos")
    print(f"Libro encontrado: {found}")
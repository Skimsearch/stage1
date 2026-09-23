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
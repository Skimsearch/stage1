import time
from downloader import download_books

books = [1342, 11, 84, 98, 1661]

strategies = ["time", "book", "batch"]

for strategy in strategies:

    start = time.time.perf_counter()

    download_books(books, strategy)

    end = time.time.perf_counter()

    total_time = end - start

    print(f"{strategy}: {total_time:.2f} segundos")
    print(f"  Tiempo total: {total_time:.2f} s")
    print(f"  Libros: {len(books)}")
    print(f"  Media por libro: {total_time / len(books):.2f} s")
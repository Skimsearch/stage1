import time
from downloader import download_books

books = [1342, 11, 84, 98, 1661]

strategies = ["time", "book", "batch"]

for strategy in strategies:

    start = time.time()

    download_books(books, strategy)

    end = time.time()

    total_time = end - start

    print(f"{strategy}: {total_time:.2f} segundos")
import time
from downloader import download_books
from pathlib import Path

books = [1342, 11, 84, 98, 1661]

strategies = ["time", "book", "batch"]

for strategy in strategies:

    start = time.perf_counter()

    download_books(books, strategy)

    end = time.perf_counter()

    total_time = end - start

    print(f"\nStrategy: {strategy}")
    print(f"  Total time: {total_time:.2f} s")
    print(f"  Books: {len(books)}")
    print(f"  Average per book: {total_time / len(books):.2f} s")


def find_book(book_id, strategy):
    if strategy == "book":
        base_path = (
            Path("datalake/book")
            / str(book_id)
        )

        header_path = base_path / f"{book_id}.header.txt"
        body_path = base_path / f"{book_id}.body.txt"

        return header_path.exists() and body_path.exists()

    elif strategy == "batch":
        batch_start = (book_id // 1000) * 1000
        batch_end = batch_start + 999

        base_path = (
            Path("datalake/batch")
            / f"{batch_start}-{batch_end}"
        )

        header_path = base_path / f"{book_id}.header.txt"
        body_path = base_path / f"{book_id}.body.txt"

        return header_path.exists() and body_path.exists()

    elif strategy == "time":
        body_matches = list(
            Path("datalake/time").rglob(
                f"{book_id}.body.txt"
            )
        )

        header_matches = list(
            Path("datalake/time").rglob(
                f"{book_id}.header.txt"
            )
        )

        return len(body_matches) > 0 and len(header_matches) > 0

    else:
        raise ValueError(f"Unknown strategy: {strategy}")
    
print("\nLOOKUP BENCHMARK")

book_id = 1342
repetitions = 1000

for strategy in strategies:

    start = time.perf_counter()

    for _ in range(repetitions):
        found = find_book(book_id, strategy)

    end = time.perf_counter()

    total_time = end - start
    average_time = total_time / repetitions

    print(f"\n{strategy}:")
    print(f"Repetitions: {repetitions}")
    print(f"Total time: {total_time:.6f} seconds")
    print(f"Average lookup time: {average_time:.8f} seconds")
    print(f"Book found: {found}")


def storage_overhead(strategy):

    root = Path("datalake") / strategy

    files = 0
    folders = 0

    for path in root.rglob("*"):

        if path.is_file():
            files += 1

        elif path.is_dir():
            folders += 1

    return files, folders


print("\nSTORAGE OVERHEAD")

for strategy in strategies:

    files, folders = storage_overhead(strategy)

    print(f"\n{strategy}:")
    print(f"Files: {files}")
    print(f"Folders: {folders}")
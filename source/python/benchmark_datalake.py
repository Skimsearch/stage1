import time
import shutil
from datetime import datetime
from pathlib import Path
from downloader import fetch_book, save_book, get_output_path, DATALAKE_PATH

BOOKS = [1342, 11, 84, 98, 1661]
STRATEGIES = ["time", "book", "batch"]
# Reuse downloader's own DATALAKE_PATH instead of redefining "datalake"
# here: downloader.py resolves it from __file__, so if the benchmark
# script lives somewhere else, a hardcoded Path("datalake") could
# silently point at a different folder than the one files were
# actually written to.
DATALAKE_ROOT = DATALAKE_PATH


# ---------------------------------------------------------------------------
# Path resolution (single source of truth, reused by every benchmark)
# ---------------------------------------------------------------------------
#
# For "book" and "batch" we ask downloader.get_output_path() directly,
# the same function save_book() uses internally, so the benchmark can
# never drift out of sync with how files are actually written.
# "time" still needs a search (resolve_time_path) because its folder
# depends on the download timestamp, which the benchmark doesn't know
# in advance.

def resolve_time_path(book_id: int) -> Path | None:
    """Find the actual folder holding book_id under the time-based
    layout by searching for its header file (cheap file, always
    written first)."""
    matches = list((DATALAKE_ROOT / "time").rglob(f"{book_id}.header.txt"))
    if matches:
        return matches[0].parent
    # Fall back to the body file, in case the header is the one missing.
    matches = list((DATALAKE_ROOT / "time").rglob(f"{book_id}.body.txt"))
    return matches[0].parent if matches else None


def check_book_state(book_id: int, strategy: str) -> str:
    """Return 'complete', 'incomplete' or 'missing'.

    'incomplete' means only one of header/body exists, which is what a
    real interrupted download looks like. Treating that the same as
    'missing' (as a plain exists()-based check does) is exactly what
    breaks recovery-behavior testing: the pipeline would re-download
    the whole book instead of just resuming the missing part."""
    if strategy == "time":
        base_path = resolve_time_path(book_id)
        if base_path is None:
            return "missing"
    else:
        base_path = get_output_path(book_id, strategy)

    header_exists = (base_path / f"{book_id}.header.txt").exists()
    body_exists = (base_path / f"{book_id}.body.txt").exists()

    if header_exists and body_exists:
        return "complete"
    if header_exists or body_exists:
        return "incomplete"
    return "missing"


def find_book(book_id: int, strategy: str) -> bool:
    """Thin wrapper kept for the lookup benchmark: True only if both
    files are present."""
    return check_book_state(book_id, strategy) == "complete"


# ---------------------------------------------------------------------------
# 1. Download / write throughput
# ---------------------------------------------------------------------------
#
# Network and disk are timed separately and on separate loops:
#
#   - Network (fetch_book) does not depend on the datalake structure at
#     all, so it is only fair to measure it ONCE, not once per strategy.
#     Repeating it 3x would just triple the same network noise and make
#     it look like part of the "cost" of a structure, when it isn't.
#   - Disk (save_book) is what actually differs between strategies, so
#     it's timed once per strategy, reusing the SAME already-downloaded
#     text for all three, so all three write exactly the same bytes.

def benchmark_download_throughput():
    print("\nDOWNLOAD / WRITE THROUGHPUT BENCHMARK")

    # --- Network phase: fetch each book's text once, time it once. ---
    fetched = {}

    start = time.perf_counter()
    for book_id in BOOKS:
        result = fetch_book(book_id)
        if result is None:
            print(f"Book {book_id} could not be parsed (markers not found)")
            continue
        fetched[book_id] = result
    network_time = time.perf_counter() - start

    print("\nnetwork (shared across strategies):")
    print(f"  Total fetch time: {network_time:.2f} s")
    print(f"  Books fetched: {len(fetched)}")
    print(f"  Average per book: {network_time / len(fetched):.2f} s")

    # --- Disk phase: write the same in-memory data with each layout. ---
    when = datetime.now()

    for strategy in STRATEGIES:
        start = time.perf_counter()
        for book_id, (header, body) in fetched.items():
            save_book(book_id, header, body, strategy, when)
        disk_time = time.perf_counter() - start

        print(f"\n{strategy} (disk write only):")
        print(f"  Total time: {disk_time:.4f} s")
        print(f"  Books: {len(fetched)}")
        print(f"  Average per book: {disk_time / len(fetched):.6f} s")


# ---------------------------------------------------------------------------
# 2. Lookup cost
# ---------------------------------------------------------------------------

def benchmark_lookup(repetitions: int = 1000):
    print("\nLOOKUP BENCHMARK")

    # Test both a book that exists and one that doesn't: a missing-book
    # lookup is the worst case for "time" (rglob scans everything and
    # still finds nothing), and that cost matters just as much for the
    # report as the best case.
    test_ids = {"existing": BOOKS[0], "missing": 999999}

    for strategy in STRATEGIES:
        print(f"\n{strategy}:")
        for label, book_id in test_ids.items():
            start = time.perf_counter()
            for _ in range(repetitions):
                found = find_book(book_id, strategy)
            total_time = time.perf_counter() - start

            print(f"  [{label}] repetitions: {repetitions}, "
                  f"total: {total_time:.6f}s, "
                  f"avg: {total_time / repetitions:.8f}s, "
                  f"found: {found}")


# ---------------------------------------------------------------------------
# 3. Storage overhead
# ---------------------------------------------------------------------------

def storage_overhead(strategy: str):
    root = DATALAKE_ROOT / strategy
    files = folders = total_size = 0

    for path in root.rglob("*"):
        if path.is_file():
            files += 1
            total_size += path.stat().st_size
        elif path.is_dir():
            folders += 1

    return files, folders, total_size


def benchmark_storage_overhead():
    print("\nSTORAGE OVERHEAD BENCHMARK")

    for strategy in STRATEGIES:
        files, folders, total_size = storage_overhead(strategy)
        print(f"\n{strategy}:")
        print(f"  Files: {files}")
        print(f"  Folders: {folders}")
        print(f"  Total size: {total_size / 1024:.2f} KB")


# ---------------------------------------------------------------------------
# 4. Incremental processing
# ---------------------------------------------------------------------------

def get_pending_books(books_list, strategy, indexed_books):
    available = {book for book in books_list if find_book(book, strategy)}
    return available - indexed_books


def benchmark_incremental_processing():
    print("\nINCREMENTAL PROCESSING BENCHMARK")

    mock_indexed_books = {11, 84}

    for strategy in STRATEGIES:
        start = time.perf_counter()
        pending = get_pending_books(BOOKS, strategy, mock_indexed_books)
        elapsed = time.perf_counter() - start

        print(f"\n{strategy}:")
        print(f"  Detection time: {elapsed:.6f} s")
        print(f"  Books ready to index: {sorted(pending)}")


# ---------------------------------------------------------------------------
# 5. Recovery behavior
# ---------------------------------------------------------------------------

def simulate_interruption(book_id: int, strategy: str) -> Path | None:
    """Simulate a pipeline crash that happened right after the header
    was written but before the body was saved: temporarily rename the
    body file so the book looks 'incomplete'. Returns the backup path
    so the caller can restore it afterwards (never delete real data)."""
    if strategy == "time":
        base_path = resolve_time_path(book_id)
    else:
        base_path = get_output_path(book_id, strategy)

    if base_path is None:
        return None

    body_path = base_path / f"{book_id}.body.txt"
    if not body_path.exists():
        return None

    backup_path = body_path.with_suffix(".txt.bak")
    shutil.move(str(body_path), str(backup_path))
    return backup_path


def restore_from_interruption(backup_path: Path | None):
    """Undo simulate_interruption so later benchmarks (or re-runs) see
    the datalake in its original, complete state."""
    if backup_path is None or not backup_path.exists():
        return
    original_path = backup_path.with_suffix("")
    shutil.move(str(backup_path), str(original_path))


def benchmark_recovery_behavior():
    print("\nRECOVERY BEHAVIOR BENCHMARK")

    corrupt_book_id = BOOKS[0]

    for strategy in STRATEGIES:
        backup_path = simulate_interruption(corrupt_book_id, strategy)

        start = time.perf_counter()

        complete, incomplete, missing = [], [], []
        for book_id in BOOKS:
            state = check_book_state(book_id, strategy)
            if state == "complete":
                complete.append(book_id)
            elif state == "incomplete":
                incomplete.append(book_id)
            else:
                missing.append(book_id)

        elapsed = time.perf_counter() - start

        print(f"\n{strategy}:")
        print(f"  Recovery detection time: {elapsed:.6f} s")
        print(f"  Complete (skip, avoid duplicate work): {complete}")
        print(f"  Incomplete (resume without duplicating): {incomplete}")
        print(f"  Missing (download from scratch): {missing}")

        # Leave the datalake exactly as we found it before moving on.
        restore_from_interruption(backup_path)


# ---------------------------------------------------------------------------

if __name__ == "__main__":
    benchmark_download_throughput()
    benchmark_lookup()
    benchmark_storage_overhead()
    benchmark_incremental_processing()
    benchmark_recovery_behavior()
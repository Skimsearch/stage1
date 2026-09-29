import sys
import time
import shutil
from pathlib import Path

from inverted_index import (
    build_inverted_index,
    save_inverted_index,
    search_term
)

from hierarchical_index import (
    save_hierarchical_index,
    search_hierarchical,
    update_hierarchical_index
)

from inverted_index_mongodb import (
    connect_mongodb,
    save_mongodb_index,
    update_mongodb_index
)


books = [1342, 11, 84, 98, 1661]

terms = ["pride", "monster", "nonexistentword"]

NEW_BOOK = 2701

PROJECT_ROOT = Path(__file__).resolve().parents[2]

JSON_PATH = (
    PROJECT_ROOT
    / "datamart"
    / "inverted_index"
    / "inverted_index.json"
)

HIERARCHICAL_PATH = (
    PROJECT_ROOT
    / "datamart"
    / "inverted_index_hierarchical"
)


def time_search(search_fn, repetitions):
    """Run search_fn() repetitions times and return (total_time, result).
    Shared by all three backends so the loop only lives in one place."""
    start = time.perf_counter()
    result = None
    for _ in range(repetitions):
        result = search_fn()
    total_time = time.perf_counter() - start
    return total_time, result


def folder_size(path):
    total_size = 0
    for file in path.rglob("*"):
        if file.is_file():
            total_size += file.stat().st_size
    return total_size


if HIERARCHICAL_PATH.exists():
    shutil.rmtree(HIERARCHICAL_PATH)

client, collection = connect_mongodb()
collection.delete_many({})
client.close()


print("INDEXING BENCHMARK")

start = time.perf_counter()
base_index = build_inverted_index(books)
build_time = time.perf_counter() - start

print(f"\nCommon build time: {build_time:.6f} seconds")


start = time.perf_counter()
save_inverted_index(base_index)
monolithic_time = time.perf_counter() - start

print(f"Monolithic storage: {monolithic_time:.6f} seconds")


start = time.perf_counter()
save_hierarchical_index(base_index)
hierarchical_time = time.perf_counter() - start

print(f"Hierarchical storage: {hierarchical_time:.6f} seconds")


start = time.perf_counter()
save_mongodb_index(base_index)
mongodb_time = time.perf_counter() - start

print(f"MongoDB storage: {mongodb_time:.6f} seconds")


print("\nSEARCH BENCHMARK")

repetitions = 1000

# Open MongoDB connection once for the whole benchmark
client, collection = connect_mongodb()

for term in terms:

    print(f"\nSearch: {term}")

    total_time, result_inverted = time_search(
        lambda: search_term(base_index, term),
        repetitions
    )
    print(
        f"Inverted: "
        f"total={total_time:.6f} s, "
        f"avg={total_time / repetitions:.8f} s "
        f"-> {result_inverted}"
    )

    total_time, result_hierarchical = time_search(
        lambda: search_hierarchical(term),
        repetitions
    )
    print(
        f"Hierarchical: "
        f"total={total_time:.6f} s, "
        f"avg={total_time / repetitions:.8f} s "
        f"-> {result_hierarchical}"
    )

    def mongo_lookup():
        document = collection.find_one({"term": term.lower()})
        return document["postings"] if document is not None else []

    total_time, result_mongodb = time_search(mongo_lookup, repetitions)
    print(
        f"MongoDB: "
        f"total={total_time:.6f} s, "
        f"avg={total_time / repetitions:.8f} s "
        f"-> {result_mongodb}"
    )

client.close()


print("\nDISK USAGE BENCHMARK")

json_size = JSON_PATH.stat().st_size

hierarchical_size = folder_size(
    HIERARCHICAL_PATH
)

# MongoDB disk usage
client, collection = connect_mongodb()

stats = collection.database.command(
    "collStats",
    collection.name
)

mongodb_size = (
    stats["storageSize"]
    + stats["totalIndexSize"]
)

client.close()


print(
    f"\nMonolithic JSON: "
    f"{json_size / 1024:.2f} KB"
)

print(
    f"Hierarchical index: "
    f"{hierarchical_size / 1024:.2f} KB"
)

print(
    f"MongoDB: "
    f"{mongodb_size / 1024:.2f} KB"
)


print("\nMEMORY USAGE BENCHMARK")

monolithic_memory = sum(
    sys.getsizeof(term) + sys.getsizeof(postings) + sum(sys.getsizeof(p) for p in postings)
    for term, postings in base_index.items()
)

print(f"Monolithic in-memory index: {monolithic_memory / 1024:.2f} KB (entire index resident)")
print("Hierarchical: negligible resident memory (reads one term file per query)")
print("MongoDB: negligible resident memory (server-side storage, client only holds query results)")


print("\nUPDATE PERFORMANCE BENCHMARK")


new_index = build_inverted_index([NEW_BOOK])


# Inverted index

start = time.perf_counter()

for term, book_ids in new_index.items():

    if term not in base_index:
        base_index[term] = []

    for book_id in book_ids:

        if book_id not in base_index[term]:
            base_index[term].append(book_id)

save_inverted_index(base_index)

end = time.perf_counter()

print(
    f"\nInverted index update: "
    f"{end - start:.6f} seconds"
)


# Hierarchical index

start = time.perf_counter()

update_hierarchical_index(new_index)

end = time.perf_counter()

print(
    f"Hierarchical index update: "
    f"{end - start:.6f} seconds"
)


# MongoDB

start = time.perf_counter()

update_mongodb_index(new_index)

end = time.perf_counter()

print(
    f"MongoDB index update: "
    f"{end - start:.6f} seconds"
)

print("\nSCALABILITY BENCHMARK")

datasets = [
    [1342],
    [1342, 11, 84],
    [1342, 11, 84, 98, 1661],
    [1342, 11, 84, 98, 1661, 2701]
]

for dataset in datasets:

    print(f"\nBooks: {len(dataset)}")

    # Build
    start = time.perf_counter()

    index = build_inverted_index(dataset)

    build_time = time.perf_counter() - start

    print(
        f"Unique terms: "
        f"{len(index)}"
    )
    print(
        f"Build time: "
        f"{build_time:.6f} seconds"
    )


    # Monolithic
    start = time.perf_counter()

    save_inverted_index(index)

    elapsed = time.perf_counter() - start

    print(
        f"Monolithic storage: "
        f"{elapsed:.6f} seconds"
    )


    # Hierarchical
    if HIERARCHICAL_PATH.exists():
        shutil.rmtree(HIERARCHICAL_PATH)

    start = time.perf_counter()

    save_hierarchical_index(index)

    elapsed = time.perf_counter() - start

    print(
        f"Hierarchical storage: "
        f"{elapsed:.6f} seconds"
    )


    # MongoDB
    start = time.perf_counter()

    save_mongodb_index(index)

    elapsed = time.perf_counter() - start

    print(
        f"MongoDB storage: "
        f"{elapsed:.6f} seconds"
    )
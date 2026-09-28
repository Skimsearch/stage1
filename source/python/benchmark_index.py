import time

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
    save_mongodb_index,
    search_mongodb,
    update_mongodb_index
)


books = [1342, 11, 84, 98, 1661]

terms = ["pride", "monster", "nonexistentword"]

NEW_BOOK = 2701


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


print("\nBENCHMARK DE BÚSQUEDA")


for term in terms:

    print(f"\nBúsqueda: {term}")

    # Inverted index

    start = time.perf_counter()

    result = search_term(base_index, term)
    end = time.perf_counter()

    print(
        f"Inverted: {end - start:.6f} s "
        f"-> {result}"
    )


    # Hierarchical index

    start = time.perf_counter()

    result = search_hierarchical(term)

    end = time.perf_counter()

    print(
        f"Hierarchical: {end - start:.6f} s "
        f"-> {result}"
    )


    # MongoDB

    start = time.perf_counter()

    result = search_mongodb(term)

    end = time.perf_counter()

    print(
        f"MongoDB: {end - start:.6f} s "
        f"-> {result}"
    )

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
    f"{end - start:.6f} segundos"
)


# Hierarchical index

start = time.perf_counter()

update_hierarchical_index(new_index)

end = time.perf_counter()

print(
    f"Hierarchical index update: "
    f"{end - start:.6f} segundos"
)


# MongoDB

start = time.perf_counter()

update_mongodb_index(new_index)

end = time.perf_counter()

print(
    f"MongoDB index update: "
    f"{end - start:.6f} segundos"
)
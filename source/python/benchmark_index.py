import time

new_book = 2701


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
    search_mongodb
)

from inverted_index_mongodb import (
    save_mongodb_index,
    search_mongodb,
    update_mongodb_index
)


books = [1342, 11, 84, 98, 1661]

terms = ["pride", "monster", "nonexistentword"]


print("BENCHMARK DE INDEXACIÓN")

start = time.perf_counter()

inverted_index = build_inverted_index(books)
save_inverted_index(inverted_index)

end = time.perf_counter()

print(
    f"\nInverted index: "
    f"{end - start:.6f} segundos"
)


start = time.perf_counter()

hierarchical_index = build_inverted_index(books)
save_hierarchical_index(hierarchical_index)

end = time.perf_counter()

print(
    f"Hierarchical index: "
    f"{end - start:.6f} segundos"
)


start = time.perf_counter()

mongodb_index = build_inverted_index(books)
save_mongodb_index(mongodb_index)

end = time.perf_counter()

print(
    f"MongoDB index: "
    f"{end - start:.6f} segundos"
)


print("\nBENCHMARK DE BÚSQUEDA")


for term in terms:

    print(f"\nBúsqueda: {term}")

    # Inverted index

    start = time.perf_counter()

    result = search_term(inverted_index, term)

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

new_book = 2701

new_index = build_inverted_index([new_book])


# Inverted index

start = time.perf_counter()

for term, book_ids in new_index.items():

    if term not in inverted_index:
        inverted_index[term] = []

    for book_id in book_ids:

        if book_id not in inverted_index[term]:
            inverted_index[term].append(book_id)

save_inverted_index(inverted_index)

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
from pymongo import MongoClient
from inverted_index import build_inverted_index


MONGO_URI = "mongodb://localhost:27017/"
DATABASE_NAME = "stage1"
COLLECTION_NAME = "inverted_index"


def connect_mongodb():

    client = MongoClient(MONGO_URI)

    database = client[DATABASE_NAME]

    collection = database[COLLECTION_NAME]

    return client, collection


def save_mongodb_index(inverted_index):

    client, collection = connect_mongodb()

    collection.delete_many({})

    documents = []

    for term, book_ids in inverted_index.items():

        documents.append({
            "term": term,
            "postings": book_ids
        })

    if documents:
        collection.insert_many(documents)

    collection.create_index("term", unique=True)

    client.close()


def search_mongodb(term):

    client, collection = connect_mongodb()

    document = collection.find_one({
        "term": term.lower()
    })

    client.close()

    if document is None:
        return []

    return document["postings"]


if __name__ == "__main__":

    books = [1342, 11, 84, 98, 1661]

    inverted_index = build_inverted_index(books)

    save_mongodb_index(inverted_index)

    print("MongoDB index created")

    print("pride:", search_mongodb("pride"))
    print("monster:", search_mongodb("monster"))
    print("nonexistentword:", search_mongodb("nonexistentword"))
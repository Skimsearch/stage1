package ulog;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
//import java.nio.file.Paths;
import java.util.*;
import java.util.stream.Stream;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.UpdateOptions;
import com.mongodb.client.model.Updates;
import org.bson.Document;

import static com.mongodb.client.model.Filters.eq;


public class IndexResourceBenchmark {

    private static final Path DATAMART_ROOT = IndexBuilder.resolveDatamartRoot();
    private static final Path JSON_PATH = DATAMART_ROOT.resolve("inverted_index").resolve("inverted_index.json");
    private static final Path HIER_PATH = DATAMART_ROOT.resolve("inverted_index_hierarchical");

    private static final Gson GSON = new GsonBuilder().create();

    private static void saveMonolithic(Map<String, List<Integer>> index) throws IOException {
        Files.createDirectories(JSON_PATH.getParent());
        Files.writeString(JSON_PATH, GSON.toJson(index), StandardCharsets.UTF_8);
    }

    private static void saveHierarchical(Map<String, List<Integer>> index) throws IOException {
        if (Files.exists(HIER_PATH)) {
            try (Stream<Path> walk = Files.walk(HIER_PATH)) {
                walk.sorted(Comparator.reverseOrder()).forEach(p -> {
                    try {
                        Files.delete(p);
                    } catch (IOException ignored) {}
                });
            }
        }
        Files.createDirectories(HIER_PATH);

        for (Map.Entry<String, List<Integer>> entry : index.entrySet()) {
            writeTermFile(entry.getKey(), entry.getValue());
        }
    }

    private static void writeTermFile(String term, List<Integer> postings) throws IOException {
        String firstLetter = term.substring(0, 1).toUpperCase(Locale.ROOT);
        Path dir = HIER_PATH.resolve(firstLetter);
        Files.createDirectories(dir);

        StringBuilder sb = new StringBuilder();
        for (int id : postings) {
            sb.append(id).append("\n");
        }
        Files.writeString(dir.resolve(term + ".txt"), sb.toString(), StandardCharsets.UTF_8);
    }

    private static void saveMongo(Map<String, List<Integer>> index, MongoCollection<Document> collection) {
        collection.deleteMany(new Document());

        List<Document> docs = new ArrayList<>();
        for (Map.Entry<String, List<Integer>> entry : index.entrySet()) {
            docs.add(new Document("term", entry.getKey()).append("postings", entry.getValue()));
        }
        if (!docs.isEmpty()) {
            collection.insertMany(docs);
        }
    }

    // Disk usage
    private static long folderSize(Path root) throws IOException {
        if (!Files.exists(root)) {
            return 0;
        }
        long[] total = {0};
        try (Stream<Path> walk = Files.walk(root)) {
            walk.filter(Files::isRegularFile).forEach(p -> {
                try {
                    total[0] += Files.size(p);
                } catch (IOException ignored) {}
            });
        }
        return total[0];
    }

    private static void benchmarkDiskUsage(MongoDatabase db) throws IOException {
        System.out.println("\nDISK USAGE BENCHMARK");

        long jsonSize = Files.exists(JSON_PATH) ? Files.size(JSON_PATH) : 0;
        long hierSize = folderSize(HIER_PATH);

        Document stats = db.runCommand(new Document("collStats", "inverted_index"));
        long mongoSize = stats.get("storageSize", Number.class).longValue()
                + stats.get("totalIndexSize", Number.class).longValue();

        System.out.printf("Monolithic JSON: %.2f KB%n", jsonSize / 1024.0);
        System.out.printf("Hierarchical index: %.2f KB%n", hierSize / 1024.0);
        System.out.printf("MongoDB: %.2f KB%n", mongoSize / 1024.0);
    }

    // Memory usage
    private static void benchmarkMemoryUsage(Map<String, List<Integer>> index) {
        System.out.println("\nMEMORY USAGE BENCHMARK");

        long bytes = 0;
        for (Map.Entry<String, List<Integer>> entry : index.entrySet()) {
            bytes += entry.getKey().length() * 2L; // UTF-16 chars
            bytes += entry.getValue().size() * 16L; // boxed Integer overhead estimate
        }

        System.out.printf("Monolithic in-memory index: %.2f KB (entire index resident)%n", bytes / 1024.0);
        System.out.println("Hierarchical: negligible resident memory (reads one term file per query)");
        System.out.println("MongoDB: negligible resident memory (server-side storage, client only holds query results)");
    }

    // Update performance
    private static void benchmarkUpdatePerformance(MongoCollection<Document> collection) throws IOException {
        System.out.println("\nUPDATE PERFORMANCE BENCHMARK");

        List<Integer> baseBooks = IndexBuilder.ALL_BOOKS.subList(0, IndexBuilder.ALL_BOOKS.size() - 1);
        Map<String, List<Integer>> baseIndex = IndexBuilder.buildInvertedIndex(baseBooks);
        saveMonolithic(baseIndex);
        saveHierarchical(baseIndex);
        saveMongo(baseIndex, collection);

        Map<String, List<Integer>> newIndex = IndexBuilder.buildInvertedIndex(
                Collections.singletonList(IndexBuilder.NEW_BOOK));

        // Monolithic
        long start = System.nanoTime();
        for (Map.Entry<String, List<Integer>> entry : newIndex.entrySet()) {
            List<Integer> postings = baseIndex.computeIfAbsent(entry.getKey(), k -> new ArrayList<>());
            for (int bookId : entry.getValue()) {
                if (!postings.contains(bookId)) {
                    postings.add(bookId);
                }
            }
        }
        saveMonolithic(baseIndex);
        System.out.printf("Monolithic index update: %.6f seconds%n", (System.nanoTime() - start) / 1e9);

        // Hierarchical
        start = System.nanoTime();
        for (Map.Entry<String, List<Integer>> entry : newIndex.entrySet()) {
            String term = entry.getKey();
            Path termFile = HIER_PATH.resolve(term.substring(0, 1).toUpperCase(Locale.ROOT)).resolve(term + ".txt");

            List<Integer> postings = new ArrayList<>();
            if (Files.exists(termFile)) {
                for (String line : Files.readAllLines(termFile)) {
                    if (!line.trim().isEmpty()) {
                        postings.add(Integer.parseInt(line.trim()));
                    }
                }
            }
            for (int bookId : entry.getValue()) {
                if (!postings.contains(bookId)) {
                    postings.add(bookId);
                }
            }
            writeTermFile(term, postings);
        }
        System.out.printf("Hierarchical index update: %.6f seconds%n", (System.nanoTime() - start) / 1e9);

        // MongoDB
        start = System.nanoTime();
        for (Map.Entry<String, List<Integer>> entry : newIndex.entrySet()) {
            for (int bookId : entry.getValue()) {
                collection.updateOne(
                        eq("term", entry.getKey()),
                        Updates.addToSet("postings", bookId),
                        new UpdateOptions().upsert(true)
                );
            }
        }
        System.out.printf("MongoDB index update: %.6f seconds%n", (System.nanoTime() - start) / 1e9);
    }

    // Scalability
    private static void benchmarkScalability(MongoCollection<Document> collection) throws IOException {
        System.out.println("\nSCALABILITY BENCHMARK");

        int[] datasetSizes = {1, 3, 5, 6};

        for (int count : datasetSizes) {
            List<Integer> subset = IndexBuilder.ALL_BOOKS.subList(0, count);
            System.out.println("\nBooks: " + count);

            long start = System.nanoTime();
            Map<String, List<Integer>> index = IndexBuilder.buildInvertedIndex(subset);
            double buildTime = (System.nanoTime() - start) / 1e9;

            System.out.println("Unique terms: " + index.size());
            System.out.printf("Build time: %.6f seconds%n", buildTime);

            start = System.nanoTime();
            saveMonolithic(index);
            System.out.printf("Monolithic storage: %.6f seconds%n", (System.nanoTime() - start) / 1e9);

            start = System.nanoTime();
            saveHierarchical(index);
            System.out.printf("Hierarchical storage: %.6f seconds%n", (System.nanoTime() - start) / 1e9);

            start = System.nanoTime();
            saveMongo(index, collection);
            System.out.printf("MongoDB storage: %.6f seconds%n", (System.nanoTime() - start) / 1e9);
        }
    }

    public static void main(String[] args) throws Exception {
        MongoClient client = MongoClients.create("mongodb://localhost:27017/");
        MongoDatabase db = client.getDatabase("stage1");
        MongoCollection<Document> collection = db.getCollection("inverted_index");

        try {
            Map<String, List<Integer>> fullIndex = IndexBuilder.buildInvertedIndex(IndexBuilder.ALL_BOOKS);
            saveMonolithic(fullIndex);
            saveHierarchical(fullIndex);
            saveMongo(fullIndex, collection);

            benchmarkDiskUsage(db);
            benchmarkMemoryUsage(fullIndex);
            benchmarkUpdatePerformance(collection);
            benchmarkScalability(collection);
        } finally {
            client.close();
        }
    }
}
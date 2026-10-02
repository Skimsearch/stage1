package ulog;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import org.openjdk.jmh.annotations.*;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.lang.reflect.Type;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.concurrent.TimeUnit;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import org.bson.Document;

import static com.mongodb.client.model.Filters.eq;


@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@State(Scope.Benchmark)
@Warmup(iterations = 2, time = 1, timeUnit = TimeUnit.SECONDS)
@Measurement(iterations = 5, time = 1, timeUnit = TimeUnit.SECONDS)
@Fork(1)
public class InvertedIndexBenchmark {

    private String hierDir;
    private Map<String, List<Integer>> memoryJsonIndex;
    private MongoClient mongoClient;
    private MongoCollection<Document> mongoCollection;

    @Param({
        "pride",
        "monster",
        "nonexistentword"
    })
    public String queryWord;

    @Setup(Level.Trial)
    public void setup() throws IOException {
        Path pJson = Paths.get("../../datamart/inverted_index/inverted_index.json");
        if (!Files.exists(pJson)) {
            pJson = Paths.get("datamart/inverted_index/inverted_index.json");
        }
        String jsonPath = pJson.toAbsolutePath().toString();

        Path pHier = Paths.get("../../datamart/inverted_index_hierarchical");
        if (!Files.exists(pHier)) {
            pHier = Paths.get("datamart/inverted_index_hierarchical");
        }
        hierDir = pHier.toAbsolutePath().toString();

        Gson gson = new Gson();
        Type type = new TypeToken<Map<String, List<Integer>>>(){}.getType();
        try (BufferedReader reader = new BufferedReader(new FileReader(jsonPath))) {
            memoryJsonIndex = gson.fromJson(reader, type);
        }

        mongoClient = MongoClients.create("mongodb://localhost:27017/");

        // NOTE: this line was missing before, which left mongoCollection
        // null and made queryMongoDB() throw a NullPointerException at
        // runtime. Database/collection names match inverted_index_mongodb.py
        // (DATABASE_NAME="stage1", COLLECTION_NAME="inverted_index").
        mongoCollection = mongoClient
            .getDatabase("stage1")
            .getCollection("inverted_index");
    }

    @Benchmark
    public List<Integer> queryMonolithicJson() {

        List<Integer> postings =
            memoryJsonIndex.get(queryWord);

        if (postings == null) {
            return Collections.emptyList();
        }

        return postings;
    }

    @Benchmark
    public List<Integer> queryHierarchicalTxt()
            throws IOException {

        String firstLetter =
            queryWord.substring(0, 1).toUpperCase();

        Path termFile = Paths.get(
            hierDir,
            firstLetter,
            queryWord + ".txt"
        );

        if (!Files.exists(termFile)) {
            return Collections.emptyList();
        }

        List<Integer> postings = new ArrayList<>();

        List<String> lines =
            Files.readAllLines(termFile);

        for (String line : lines) {

            if (!line.trim().isEmpty()) {
                postings.add(
                    Integer.parseInt(line.trim())
                );
            }
        }

        return postings;
    }

    @Benchmark
    public List<Integer> queryMongoDB() {

        Document document = mongoCollection
            .find(eq("term", queryWord))
            .first();

        if (document == null) {
            return Collections.emptyList();
        }

        List<Integer> postings =
            document.getList(
                "postings",
                Integer.class
            );

        if (postings == null) {
            return Collections.emptyList();
        }

        return postings;
    }

    @TearDown(Level.Trial)
    public void tearDown() {

        if (mongoClient != null) {
            mongoClient.close();
        }
    }
}
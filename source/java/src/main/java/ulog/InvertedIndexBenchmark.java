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

import java.util.regex.Matcher;
import java.util.regex.Pattern;

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

    private String jsonPath;
    private String hierDir;
    private Map<String, List<Integer>> memoryJsonIndex;
    private MongoClient mongoClient;
    private MongoCollection<Document> mongoCollection;
    private static final List<Integer> BOOKS = Arrays.asList(
        1342,
        11,
        84,
        98,
        1661
    );

    @Param({
        "pride",
        "monster",
        "nonexistentword"
    })
    public String queryWord;

    private List<String> tokenize(String text) {

        List<String> tokens = new ArrayList<>();

        Matcher matcher = Pattern
            .compile("\\b[a-zA-Z]+\\b")
            .matcher(text.toLowerCase(Locale.ROOT));

        while (matcher.find()) {
            tokens.add(matcher.group());
        }

        return tokens;
    }

    private Map<String, List<Integer>> buildInvertedIndex(List<Integer> bookIds) throws IOException {
        Map<String, List<Integer>> index =
            new HashMap<>();

        for (int bookId : bookIds) {

            Path bodyPath = Paths.get(
                "../../datalake/book/"
                + bookId
                + "/"
                + bookId
                + ".body.txt"
            );

            if (!Files.exists(bodyPath)) {
                bodyPath = Paths.get(
                    "datalake/book/"
                    + bookId
                    + "/"
                    + bookId
                    + ".body.txt"
                );
            }

            if (!Files.exists(bodyPath)) {
                continue;
            }

            String text = Files.readString(bodyPath);

            Set<String> uniqueTokens =
                new HashSet<>(tokenize(text));

            for (String token : uniqueTokens) {

                index
                    .computeIfAbsent(
                        token,
                        key -> new ArrayList<>()
                    )
                    .add(bookId);
            }
        }

        return index;
    }
    @Setup(Level.Trial)
    public void setup() throws IOException {
        Path pJson = Paths.get("../../datamart/inverted_index/inverted_index.json");
        if (!Files.exists(pJson)) {
            pJson = Paths.get("datamart/inverted_index/inverted_index.json");
        }
        jsonPath = pJson.toAbsolutePath().toString();

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
        mongoClient = MongoClients.create(
        "mongodb://localhost:27017/"
    );

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

    @Benchmark
    public Map<String, List<Integer>> buildIndexBenchmark()
            throws IOException {

        return buildInvertedIndex(BOOKS);
    }

    @TearDown(Level.Trial)
    public void tearDown() {

        if (mongoClient != null) {
            mongoClient.close();
        }
    }
}